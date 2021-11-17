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

#pragma once

#include <graphqlservice/GraphQLService.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <set>

#include <graphql_vss_server_libs/support/debug.hpp>
#include <graphql_vss_server_libs/support/log.hpp>
// Added to backward compatibility with older versions of DLT Daemon
#include <graphql_vss_server_libs/support/dlt_helpers.hpp>

#include "graphqlconnection.hpp"

#include "authorizer.hpp"

#include "graphql_vss_server_libs-protocol_export.h"

using namespace graphql;

class GraphQLServer
{
public:
    GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT GraphQLServer(websocketpp::lib::asio::io_service* io_service,
        Authorizer& authorizer, service::Request& executableSchema);
    GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT void startAccept(
        uint16_t port, bool reuseAddress = true, std::function<void(void)> onReady = nullptr);

    // This will schedule an ASIO task to be executed, keep the server alive until it finishes
    GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT void
    stopListening(std::function<void(void)> onStopped = nullptr);

    GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT void garbageCollect() noexcept;

    GraphQLServer(GraphQLServer const&) = delete;
    GraphQLServer(GraphQLServer&&) = delete;

private:
    // DLT logger, based on websocketpp/logger/syslog.hpp
    template <typename concurrency, typename names>
    class WebSocketLoggerDlt : public websocketpp::log::basic<concurrency, names>
    {
    public:
        typedef websocketpp::log::basic<concurrency, names> base;

        WebSocketLoggerDlt<concurrency, names>(websocketpp::log::channel_type_hint::value hint =
                                                   websocketpp::log::channel_type_hint::access)
            : websocketpp::log::basic<concurrency, names>(hint)
            , m_channel_type_hint(hint)
        {
        }

        WebSocketLoggerDlt<concurrency, names>(websocketpp::log::level channels,
            websocketpp::log::channel_type_hint::value hint =
                websocketpp::log::channel_type_hint::access)
            : websocketpp::log::basic<concurrency, names>(channels, hint)
            , m_channel_type_hint(hint)
        {
        }

        void write(websocketpp::log::level channel, std::string const& msg)
        {
            write(channel, msg.c_str(), msg.size());
        }

        void write(websocketpp::log::level channel, char const* msg)
        {
            write(channel, msg, strlen(msg));
        }

        void write(websocketpp::log::level channel, char const* msg, size_t len)
        {
            scoped_lock_type lock(base::m_lock);
            if (!this->dynamic_test(channel))
                return;
            dbg(COLOR_BLUE "WebSocket [" << names::channel_name(channel) << "] "
                                         << std::string_view(msg, len));
            DLT_LOG(dltWebSocket,
                getDltLevel(channel),
                DLT_CSTRING("channel="),
                DLT_STRING(names::channel_name(channel)),
                DLT_CSTRING(" message="),
                DLT_SIZED_UTF8(msg, len));
        }

    private:
        typedef typename base::scoped_lock_type scoped_lock_type;
        websocketpp::log::channel_type_hint::value m_channel_type_hint;

        constexpr DltLogLevelType getDltLevel(websocketpp::log::level channel) const
        {
            if (m_channel_type_hint == websocketpp::log::channel_type_hint::access)
                return getDltLevelAccess(channel);
            else
                return getDltLevelError(channel);
        }

        constexpr DltLogLevelType getDltLevelError(websocketpp::log::level channel) const
        {
            switch (channel)
            {
                case websocketpp::log::elevel::devel:
                    return DLT_LOG_VERBOSE;
                case websocketpp::log::elevel::library:
                    return DLT_LOG_DEBUG;
                case websocketpp::log::elevel::info:
                    return DLT_LOG_INFO;
                case websocketpp::log::elevel::warn:
                    return DLT_LOG_WARN;
                case websocketpp::log::elevel::rerror:
                    return DLT_LOG_ERROR;
                case websocketpp::log::elevel::fatal:
                    return DLT_LOG_FATAL;
                default:
                    return DLT_LOG_DEBUG;
            }
        }

        constexpr DltLogLevelType getDltLevelAccess(websocketpp::log::level) const
        {
            return DLT_LOG_INFO;
        }
    };

    // Based on
    // https://github.com/zaphoyd/websocketpp/blob/master/examples/enriched_storage/enriched_storage.cpp
    struct WebSocketConfig : public websocketpp::config::asio
    {
        // pull default settings from our core config
        typedef websocketpp::config::asio core;
        typedef core::concurrency_type concurrency_type;
        typedef core::request_type request_type;
        typedef core::response_type response_type;
        typedef core::message_type message_type;
        typedef core::con_msg_manager_type con_msg_manager_type;
        typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;
        typedef WebSocketLoggerDlt<concurrency_type, websocketpp::log::elevel> elog_type;
        typedef WebSocketLoggerDlt<concurrency_type, websocketpp::log::alevel> alog_type;
        typedef core::rng_type rng_type;
        typedef core::transport_type transport_type;
        typedef core::endpoint_base endpoint_base;
        // Set a custom connection_base class
        typedef GraphQLConnection connection_base;
    };
    typedef websocketpp::server<WebSocketConfig> WebSocketServer;
    typedef WebSocketServer::connection_ptr WebSocketConnectionPtr;
    typedef WebSocketServer::message_ptr WebSocketMessagePtr;

    WebSocketServer m_webSocketServer;
    // WARNING: only access these from the main thread, use defer() to do that.
    std::set<WebSocketConnectionPtr> m_connections;
    Authorizer& m_authorizer;
    service::Request& m_executableSchema;
    SingletonStorage m_singletonStorage;

    boost::asio::thread_pool m_threadPool;

    std::unique_ptr<boost::asio::steady_timer> m_notifyTimer;
    std::map<std::string, std::shared_ptr<GraphQLNotifyTriggers>> m_pendingNotifyTriggers;
    const GraphQLNotifyTriggers* m_currentNotificationTrigger;
    static constexpr std::chrono::milliseconds notifyAfter = std::chrono::milliseconds(1);

    std::unique_ptr<boost::asio::steady_timer> m_garbageCollectTimer;
    static constexpr std::chrono::milliseconds garbageCollectAfter = std::chrono::seconds(
#ifdef GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_GC_TIMEOUT
        GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_GC_TIMEOUT
#elif defined(GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG)
        10 /* force it more often in debug */
#else
        300
#endif
    );

    void defer(std::function<void(void)>&& runOnMainThread) noexcept;
    void offloadWork(std::function<void(void)>&& runOnThreadPool) noexcept;
    std::unique_ptr<boost::asio::steady_timer> createTimer() noexcept;

    // NOTE: shared because notifyInMainThread() + bind needs
    void notify(std::shared_ptr<GraphQLNotifyTriggers>&& triggers) noexcept;
    void notifyInMainThread(std::shared_ptr<GraphQLNotifyTriggers> triggers) noexcept;
    void deliverNotifications() noexcept;
    const GraphQLNotifyTriggers& currentNotificationTriggers() const;

    void scheduleGarbageCollectIfNeeded() noexcept;

    void
    onConnectionStart(WebSocketConnectionPtr con, std::function<void(response::Value&&)>&& onReply,
        std::function<void(void)>&& terminate) noexcept;
    void onConnectionDone(WebSocketConnectionPtr con) noexcept;

    // Http Lifecycle
    void onHttp(websocketpp::connection_hdl hdl) noexcept;
    void onHttpReply(WebSocketConnectionPtr con, response::Value&& response) noexcept;

    // WebSocket Lifecycle
    void onWebSocketOpen(websocketpp::connection_hdl hdl) noexcept;
    void onWebSocketReply(WebSocketConnectionPtr con, response::Value&& response) noexcept;
    void onWebSocketTerminate(WebSocketConnectionPtr con) noexcept;
    void onWebSocketClose(websocketpp::connection_hdl hdl) noexcept;
    void onWebSocketMessage(websocketpp::connection_hdl hdl, WebSocketMessagePtr msg) noexcept;
    bool onWebSocketValidate(websocketpp::connection_hdl hdl) noexcept;
};
