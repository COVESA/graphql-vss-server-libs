// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi),
//   Author: Leandro Ferlin (leandroferlin@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <graphqlservice/JSONResponse.h>

#include <graphql_vss_server_libs/support/debug.hpp>
#include <graphql_vss_server_libs/support/log.hpp>
// Added to backward compatibility with older versions of DLT Daemon
#include <graphql_vss_server_libs/support/dlt_helpers.hpp>

#include "messagetypes.hpp"
#include "response_helpers.hpp"

#include "graphqlserver.hpp"


GraphQLServer::GraphQLServer(websocketpp::lib::asio::io_service* io_service, Authorizer& authorizer,
    service::Request& executableSchema)
    : m_authorizer(authorizer)
    , m_executableSchema(executableSchema)
{
#if GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
    m_webSocketServer.set_access_channels(websocketpp::log::alevel::all);
    m_webSocketServer.set_error_channels(websocketpp::log::elevel::all);
#else
    m_webSocketServer.clear_access_channels(websocketpp::log::alevel::all);
    m_webSocketServer.set_access_channels(websocketpp::log::alevel::access_core);
    m_webSocketServer.clear_error_channels(websocketpp::log::elevel::devel);
#endif

    m_webSocketServer.init_asio(io_service);

    m_webSocketServer.set_http_handler(
        std::bind(&GraphQLServer::onHttp, this, std::placeholders::_1));

    m_webSocketServer.set_open_handler(
        std::bind(&GraphQLServer::onWebSocketOpen, this, std::placeholders::_1));
    m_webSocketServer.set_close_handler(
        std::bind(&GraphQLServer::onWebSocketClose, this, std::placeholders::_1));
    m_webSocketServer.set_message_handler(std::bind(&GraphQLServer::onWebSocketMessage,
        this,
        std::placeholders::_1,
        std::placeholders::_2));
    m_webSocketServer.set_validate_handler(
        std::bind(&GraphQLServer::onWebSocketValidate, this, std::placeholders::_1));
}

void GraphQLServer::startAccept(uint16_t port, bool reuseAddress, std::function<void(void)> onReady)
{
    dbg(COLOR_BG_BLUE << "run server at port=" << port << " reuseAddress=" << reuseAddress);
    m_webSocketServer.set_reuse_addr(reuseAddress);
    m_webSocketServer.listen(port);
    m_webSocketServer.start_accept();

    if (onReady)
        defer(std::move(onReady));

    DLT_LOG(dltServer,
        DLT_LOG_INFO,
        DLT_CSTRING("start accepting HTTP requests at port="),
        DLT_UINT16(port));
}

void GraphQLServer::stopListening(std::function<void(void)> onStopped)
{
    DLT_LOG(dltServer,
        DLT_LOG_INFO,
        DLT_CSTRING("stop listening, pending connections="),
        m_connections.size());

    dbg(COLOR_BG_BLUE << "stop server (pending connections=" << m_connections.size() << ")");
    m_webSocketServer.stop_listening();

    defer([this, onStopped] {
        dbg(COLOR_BG_BLUE << "wait any pending server work to complete...");

        if (m_connections.size() > 0)
            DLT_LOG(dltServer,
                DLT_LOG_INFO,
                DLT_CSTRING("stop pending connections="),
                DLT_UINT64(m_connections.size()));

        for (auto& con : m_connections)
            con->stop();

        DLT_LOG(dltServer, DLT_LOG_INFO, DLT_CSTRING("join thread pool"));

        // wait outstanding jobs to complete, they may change m_connections
        m_threadPool.join();

        if (m_notifyTimer)
        {
            dbg(COLOR_BG_BLUE << "server stopped with pending notifications, ignore them");
            m_notifyTimer.reset();
        }
        m_pendingNotifyTriggers.clear();

        if (m_garbageCollectTimer)
        {
            dbg(COLOR_BG_BLUE << "server stopped with pending garbage collect, do it now");
            m_garbageCollectTimer.reset();
        }
        m_singletonStorage.clear(); // gc + detach references in use

        auto pendingConnections = std::move(m_connections);
        for (auto& con : pendingConnections)
            m_webSocketServer.close(con, websocketpp::close::status::going_away, "server stopped");
        pendingConnections.clear();

        dbg(COLOR_BG_BLUE << "server work is done!");

        if (onStopped)
            onStopped();

        DLT_LOG(dltServer, DLT_LOG_INFO, DLT_CSTRING("server stopped"));
    });
}

void GraphQLServer::defer(std::function<void(void)>&& runOnMainThread) noexcept
{
    boost::asio::defer(m_webSocketServer.get_io_service(), std::move(runOnMainThread));
}

void GraphQLServer::offloadWork(std::function<void(void)>&& runOnThreadPool) noexcept
{
    boost::asio::defer(m_threadPool, std::move(runOnThreadPool));
}

std::unique_ptr<boost::asio::steady_timer> GraphQLServer::createTimer() noexcept
{
    return std::make_unique<boost::asio::steady_timer>(m_webSocketServer.get_io_service());
}

void GraphQLServer::notify(std::shared_ptr<GraphQLNotifyTriggers>&& triggers) noexcept
{
    defer(std::bind(&GraphQLServer::notifyInMainThread,
        this,
        // NOTE: we can't bind directly to GraphQLNotifyTriggers since std::bind()
        // doesn't know it will be called only once. Then use a shared_ptr to allow it to pass
        // the checks, but it will be used by exactly one holder
        std::move(triggers)));
}

void GraphQLServer::notifyInMainThread(
    std::shared_ptr<GraphQLNotifyTriggers> otherTriggers) noexcept
{
    auto& existingTriggers = m_pendingNotifyTriggers[otherTriggers->subscriptionName];

    if (existingTriggers)
        existingTriggers->merge(*otherTriggers);
    else
    {
        existingTriggers = otherTriggers;
        m_pendingNotifyTriggers[otherTriggers->subscriptionName] = otherTriggers;
    }

    if (m_notifyTimer)
        return; // don't reschedule!

    m_notifyTimer = createTimer();
    m_notifyTimer->expires_after(notifyAfter);
    m_notifyTimer->async_wait([this](const boost::system::error_code& error) {
        if (error)
        {
            dbg(COLOR_BG_BLUE << "GraphQLServer " << this << ": timer error=" << error.message());
            return;
        }
        this->deliverNotifications();
    });
}

void GraphQLServer::deliverNotifications() noexcept
{
    m_notifyTimer.reset();
    auto triggers = std::move(m_pendingNotifyTriggers);
    for (auto& t : triggers)
    {
        m_currentNotificationTrigger = t.second.get();
        dbg(COLOR_BG_BLUE << "GraphQLServer " << this << ": deliver=" << t.first
                          << " triggers=" << m_currentNotificationTrigger->toString());
        m_executableSchema.deliver(
            std::launch::deferred,
            std::string { t.first },
            [](response::MapType::const_reference) noexcept {
                return true;
            },
            [](response::MapType::const_reference) noexcept {
                return true;
            },
            nullptr);
    }
    m_currentNotificationTrigger = nullptr;
    triggers.clear();
}

const GraphQLNotifyTriggers& GraphQLServer::currentNotificationTriggers() const
{
    if (!m_currentNotificationTrigger)
        throw std::logic_error("calling currentNotificationTriggers from outside a notification!");

    return *m_currentNotificationTrigger;
}

void GraphQLServer::scheduleGarbageCollectIfNeeded() noexcept
{
    if (m_singletonStorage.pendingGarbageCollect() == 0)
        return;
    if (m_garbageCollectTimer)
        return; // don't reschedule!

    m_garbageCollectTimer = createTimer();
    m_garbageCollectTimer->expires_after(garbageCollectAfter);
    m_garbageCollectTimer->async_wait([this](const boost::system::error_code& error) {
        if (error)
        {
            dbg(COLOR_BG_BLUE << "GraphQLServer " << this << ": timer error=" << error.message());
            return;
        }
        this->garbageCollect();
    });

    dbg(COLOR_BG_BLUE
        << "GraphQLServer " << this << ": collect garbage in "
        << std::chrono::duration_cast<std::chrono::seconds>(garbageCollectAfter).count() << "s");
}

void GraphQLServer::garbageCollect() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLServer " << this << ": collect garbage");
    DLT_LOG(dltServer, DLT_LOG_INFO, DLT_CSTRING("collect garbage"));
    m_garbageCollectTimer.reset();
    m_singletonStorage.garbageCollect();
}

void GraphQLServer::onConnectionStart(GraphQLServer::WebSocketConnectionPtr con,
    std::function<void(response::Value&&)>&& onReply,
    std::function<void(void)>&& terminate) noexcept
{
    con->setup(&m_authorizer,
        &m_executableSchema,
        &m_singletonStorage,
        { std::move(onReply),
            std::bind(&GraphQLServer::defer, this, std::placeholders::_1),
            std::bind(&GraphQLServer::offloadWork, this, std::placeholders::_1),
            std::bind(&GraphQLServer::createTimer, this),
            std::bind(&GraphQLServer::notify, this, std::placeholders::_1),
            std::bind(&GraphQLServer::currentNotificationTriggers, this),
            std::move(terminate) });

    m_connections.insert(con);
}

void GraphQLServer::onConnectionDone(GraphQLServer::WebSocketConnectionPtr con) noexcept
{
    m_connections.erase(con);
    con->tearDown();
    scheduleGarbageCollectIfNeeded();
}

void GraphQLServer::onHttp(websocketpp::connection_hdl hdl) noexcept
{
    auto con = m_webSocketServer.get_con_from_hdl(hdl);

    onConnectionStart(con,
        std::bind(&GraphQLServer::onHttpReply, this, con, std::placeholders::_1),
        nullptr);

    con->defer_http_response();
    const auto& authorization = con->get_request_header("authorization");

    DLT_LOG(dltServer,
        DLT_LOG_VERBOSE,
        DLT_CSTRING("connection="),
        DLT_PTR(con.get()),
        DLT_CSTRING(" http="),
        DLT_SIZED_UTF8(con->get_request_body().data(), con->get_request_body().size()));

    auto payload = response::parseJSON(con->get_request_body());
    con->onHttp(authorization, std::move(payload));
}

void GraphQLServer::onHttpReply(
    GraphQLServer::WebSocketConnectionPtr con, response::Value&& response) noexcept
{
    auto [type, id, payload] = response::helpers::toMessageParts(std::move(response));

    if (type == GQL_COMPLETE)
        return;

    int statusCode = websocketpp::http::status_code::value::not_implemented;
    if (type == GQL_DATA && payload.type() == response::Type::Map)
    {
        if (payload.find(GQL_DATA) != payload.end())
            statusCode = websocketpp::http::status_code::value::ok;
        else
            statusCode = websocketpp::http::status_code::value::bad_request;
    }
    else if (type == GQL_ERROR || type == GQL_CONNECTION_ERROR)
    {
        statusCode = websocketpp::http::status_code::value::bad_request;
        if (payload.type() == response::Type::Map)
        {
            const auto itr = payload.find(GQL_STATUS_CODE);
            if (itr != payload.end() && itr->second.type() == response::Type::Int)
                statusCode = itr->second.get<response::IntType>();
        }
    }

    std::string body = response::toJSON(std::move(payload));

    DLT_LOG(dltServer,
        DLT_LOG_DEBUG,
        DLT_CSTRING("connection="),
        DLT_PTR(con.get()),
        DLT_CSTRING(" reply="),
        DLT_SIZED_STRING(body.data(), body.size()),
        DLT_CSTRING(" statusCode="),
        DLT_INT(statusCode));

    defer([this, con, statusCode, body = std::move(body)] {
        con->set_status(static_cast<websocketpp::http::status_code::value>(statusCode));
        con->set_body(body);
        con->send_http_response();
        this->onConnectionDone(con);
    });
}

void GraphQLServer::onWebSocketOpen(websocketpp::connection_hdl hdl) noexcept
{
    auto con = m_webSocketServer.get_con_from_hdl(hdl);

    onConnectionStart(con,
        std::bind(&GraphQLServer::onWebSocketReply, this, con, std::placeholders::_1),
        std::bind(&GraphQLServer::onWebSocketTerminate, this, con));
}

void GraphQLServer::onWebSocketReply(
    GraphQLServer::WebSocketConnectionPtr con, response::Value&& response) noexcept
{
    std::string body = response::toJSON(std::move(response));

    DLT_LOG(dltServer,
        DLT_LOG_DEBUG,
        DLT_CSTRING("connection="),
        DLT_PTR(con.get()),
        DLT_CSTRING(" reply="),
        DLT_SIZED_STRING(body.data(), body.size()));

    defer([con, body = std::move(body)] {
        if (con->get_state() != websocketpp::session::state::value::open)
        {
            dbg(COLOR_BG_BLUE << "connection " << con.get()
                              << " already closed, ignore reply: " << body);
            return;
        }
        con->send(body);
    });
}

void GraphQLServer::onWebSocketTerminate(GraphQLServer::WebSocketConnectionPtr con) noexcept
{
    defer([con] {
        if (con->get_state() != websocketpp::session::state::value::open)
        {
            dbg(COLOR_BG_BLUE << "connection " << con.get() << " already closed, ignore terminate");
            return;
        }
        con->close(websocketpp::close::status::normal, "client requested close");
    });
}

void GraphQLServer::onWebSocketClose(websocketpp::connection_hdl hdl) noexcept
{
    auto con = m_webSocketServer.get_con_from_hdl(hdl);
    onConnectionDone(con);
}

void GraphQLServer::onWebSocketMessage(
    websocketpp::connection_hdl hdl, GraphQLServer::WebSocketMessagePtr msg) noexcept
{
    auto con = m_webSocketServer.get_con_from_hdl(hdl);

    DLT_LOG(dltServer,
        DLT_LOG_VERBOSE,
        DLT_CSTRING("connection="),
        DLT_PTR(con.get()),
        DLT_CSTRING(" ws="),
        DLT_SIZED_UTF8(msg->get_payload().data(), msg->get_payload().size()));

    con->onWebSocketMessage(response::parseJSON(msg->get_payload()));
}

bool GraphQLServer::onWebSocketValidate(websocketpp::connection_hdl hdl) noexcept
{
    static constexpr std::string_view desiredSubProtocol("graphql-ws");
    auto con = m_webSocketServer.get_con_from_hdl(hdl);
    const std::vector<std::string>& subprotocols = con->get_requested_subprotocols();
    if (subprotocols.size() == 0)
    {
        dbg(COLOR_BG_BLUE << "no sub-protocol requested, assume " << desiredSubProtocol);
        return true;
    }

    for (const auto& p : subprotocols)
    {
        if (std::string_view(p) == desiredSubProtocol)
        {
            con->select_subprotocol(p);
            return true;
        }
        dbg(COLOR_BG_BLUE << "received unusable sub-protocol=" << p);
    }

    DLT_LOG(dltServer,
        DLT_LOG_WARN,
        DLT_CSTRING("connection="),
        DLT_PTR(con.get()),
        DLT_CSTRING(": no required protocols are usable"));

    dbg(COLOR_BG_BLUE << "no required protocols are usable");
    return false;
}
