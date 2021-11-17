// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi),
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi),
//   Author: Leandro Ferlin (leandroferlin@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <graphqlservice/internal/Grammar.h>
#include <tao/pegtl/contrib/parse_tree.hpp>
// Added to backward compatibility with older versions of DLT Daemon
#include <graphql_vss_server_libs/support/dlt_helpers.hpp>

#include <graphql_vss_server_libs/support/debug.hpp>
#include <graphql_vss_server_libs/support/log.hpp>
#include <graphql_vss_server_libs/support/spinlock.hpp>

#include "messagetypes.hpp"
#include "response_helpers.hpp"
#include "subscriptiondefinitionnamevisitor.hpp"

#include "graphqlconnectionoperation.hpp"

#if GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
#define CONNECTION_OPERATION_CHECK_MAIN_THREAD checkThread(__PRETTY_FUNCTION__)

void GraphQLConnectionOperation::checkThread(const std::string_view& fn) const
{
    if (m_threadId != std::this_thread::get_id())
    {
        dbg(COLOR_BG_RED << "Calling '" << fn << "' from incorrect thread, expected " << m_threadId
                         << ", got " << std::this_thread::get_id());
        abort();
    }
}

#else
#define CONNECTION_OPERATION_CHECK_MAIN_THREAD                                                     \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#endif

class GraphQLConnectionOperationRegular final : public GraphQLConnectionOperation
{
public:
    using GraphQLConnectionOperation::GraphQLConnectionOperation;

    void start() noexcept override;
    void stop() noexcept override;

private:
    inline std::shared_ptr<GraphQLConnectionOperationRegular> getSharedPtr()
    {
        return std::static_pointer_cast<GraphQLConnectionOperationRegular>(shared_from_this());
    }

    void resolveInAThread() noexcept;
};

class GraphQLConnectionOperationSubscription final : public GraphQLConnectionOperation
{
public:
    using GraphQLConnectionOperation::GraphQLConnectionOperation;

    void start() noexcept override;
    void stop() noexcept override;
    void setSubscriptionmIntervalBetweenDeliveries(
        std::chrono::milliseconds intervalInMs) noexcept override;

protected:
    void addScopedSignalConnection(boost::signals2::scoped_connection&& con) noexcept override;
    void notify() noexcept override;

private:
    service::SubscriptionKey m_subscriptionKey;
    service::SubscriptionName m_subscriptionName;

    // GraphQLService doesn't like us to interact with the subscription from different threads
    boost::asio::thread_pool m_thread = boost::asio::thread_pool(1);

    // Rate Limiting
    std::chrono::steady_clock::duration m_intervalBetweenDeliveries;
    std::chrono::steady_clock::time_point m_lastDelivery;
    std::unique_ptr<boost::asio::steady_timer> m_deliveryTimer;
    std::future<response::Value> m_pendingDelivery;

    // Observers
    SpinLock m_scopedSignalConnectionsLock = SpinLock(this);
    std::deque<boost::signals2::scoped_connection> m_scopedSignalConnections;

    inline std::shared_ptr<GraphQLConnectionOperationSubscription> getSharedPtr()
    {
        return std::static_pointer_cast<GraphQLConnectionOperationSubscription>(shared_from_this());
    }

    void onFutureSubscription(std::future<response::Value> futureResponse) noexcept;
    void dispatchPendingDelivery() noexcept;
    void subscribeInAThread() noexcept;
    void unsubscribeInAThread() noexcept;
    void resolveSubscriptionInAThread(
        std::shared_ptr<std::future<response::Value>> futureResponse) noexcept;
    service::SubscriptionName getSubscriptionName(const peg::ast& ast) const noexcept;
    bool shouldDispatchCurrentDelivery() const noexcept;
    std::chrono::steady_clock::duration calculateTimeRemainingToDeliver() const noexcept;
};

template <typename Rule>
struct subscription_parser_selector : std::false_type
{
};
template <>
struct subscription_parser_selector<peg::operation_type> : std::true_type
{
};

static inline bool decideIsSubscription(const std::string& query)
{
    using namespace TAO_PEGTL_NAMESPACE;

    auto input = memory_input<>(query.data(), query.size(), "GraphQL");
    auto root = parse_tree::parse<peg::executable_document, subscription_parser_selector>(input);
    if (!root || root->children.empty())
        return false;

    for (auto& n : root->children)
    {
        if (n->string_view() == service::strSubscription)
            return true;
    }

    return false;
}

std::shared_ptr<GraphQLConnectionOperation>
GraphQLConnectionOperation::make(const std::string_view& id, const GraphQLRequestHandlers& handlers,
    service::Request& executableSchema, std::shared_ptr<const ClientPermissions> permissions,
    SingletonStorage& singletonStorage, response::Value&& payload)
{
    auto [query, operationName, variables] =
        response::helpers::toOperationDefinitionParts(std::move(payload));

    if (query.empty())
        throw InvalidPayload("Missing query");

    auto isSubscription = decideIsSubscription(query);
    if (isSubscription)
        return std::make_shared<GraphQLConnectionOperationSubscription>(id,
            handlers,
            executableSchema,
            permissions,
            singletonStorage,
            isSubscription,
            std::move(query),
            std::move(operationName),
            std::move(variables));
    else
        return std::make_shared<GraphQLConnectionOperationRegular>(id,
            handlers,
            executableSchema,
            permissions,
            singletonStorage,
            isSubscription,
            std::move(query),
            std::move(operationName),
            std::move(variables));
}

GraphQLConnectionOperation::GraphQLConnectionOperation(const std::string_view& id,
    const GraphQLRequestHandlers& handlers, service::Request& executableSchema,
    std::shared_ptr<const ClientPermissions> permissions, SingletonStorage& singletonStorage,
    bool isSubscription, std::string&& query, std::string&& operationName,
    response::Value&& variables)
    : GraphQLRequestState(handlers, executableSchema, permissions, singletonStorage, isSubscription)
    , m_id(id)
    , m_stopped(false)
    , m_query(std::move(query))
    , m_operationName(std::move(operationName))
    , m_variables(std::move(variables))
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id);
}

GraphQLConnectionOperation::~GraphQLConnectionOperation()
{
    dbg(COLOR_BG_BLUE << "~GraphQLConnectionOperation " << this << " id=" << m_id);
}

// GraphQLConnectionOperationRegular
void GraphQLConnectionOperationRegular::start() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id << ": start");

    DLT_LOG(dltOperation,
        DLT_LOG_DEBUG,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": start query="),
        DLT_SIZED_UTF8(m_query.data(), m_query.size()));

    CONNECTION_OPERATION_CHECK_MAIN_THREAD;

    m_handlers.offloadWork(
        std::bind(&GraphQLConnectionOperationRegular::resolveInAThread, getSharedPtr()));
}

void GraphQLConnectionOperationRegular::stop() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id << ": stop");

    DLT_LOG(dltOperation,
        DLT_LOG_DEBUG,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": stop"));

    CONNECTION_OPERATION_CHECK_MAIN_THREAD;

    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << ": already stopped, ignore another stop command!");
        return;
    }

    m_stopped = true;
}

void GraphQLConnectionOperationRegular::resolveInAThread() noexcept
{
    auto onReply = m_handlers.onReply; // copy before checking m_stopped to avoid race
    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << ": already stopped, ignore threaded resolution");
        return;
    }

    DLT_LOG(dltOperation,
        DLT_LOG_VERBOSE,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": resolve in a thread"));

    auto ast = peg::parseString(m_query);

#ifdef GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
    auto startTime = std::chrono::high_resolution_clock::now();
#endif

    auto response = m_executableSchema
                        .resolve(std::launch::deferred,
                            getSharedPtr(),
                            ast,
                            m_operationName,
                            std::move(m_variables))
                        .get();
#ifdef GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = endTime - startTime;
#else
    std::chrono::milliseconds elapsed(0);
#endif

    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << ": already stopped, ignore reply");
        return;
    }
    onReply(response::helpers::createResponse(GQL_DATA, m_id, std::move(response)));
    onReply(response::helpers::createResponse(GQL_COMPLETE, m_id, response::Value()));

    // not a subscription, thus no need to unsubscribe!
    m_stopped = true;
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << ": complete (auto-stop) [stats: elapsed="
                      << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                      << "ms, singletons=" << m_usedSingletons.size() << "]");

    if (m_failedPermissionsCheck)
    {
        dbg(COLOR_BG_RED << "GraphQLConnectionOperation " << this << " id=" << m_id
                         << ": failed permissions check");
        DLT_LOG(dltOperation,
            DLT_LOG_WARN,
            DLT_CSTRING("operation="),
            DLT_PTR(this),
            DLT_CSTRING(" id="),
            DLT_SIZED_STRING(m_id.data(), m_id.size()),
            DLT_CSTRING(": failed permissions check"));
    }
}

// GraphQLConnectionOperationSubscription
void GraphQLConnectionOperationSubscription::start() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id << ": subscribe");

    DLT_LOG(dltOperation,
        DLT_LOG_DEBUG,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": subscribe query="),
        DLT_SIZED_UTF8(m_query.data(), m_query.size()));

    CONNECTION_OPERATION_CHECK_MAIN_THREAD;

    // it will be reset when the subscription executes, but let's with 5s
    m_intervalBetweenDeliveries = std::chrono::milliseconds(5000);

    boost::asio::defer(m_thread,
        std::bind(&GraphQLConnectionOperationSubscription::subscribeInAThread, getSharedPtr()));
}

void GraphQLConnectionOperationSubscription::setSubscriptionmIntervalBetweenDeliveries(
    std::chrono::milliseconds intervalBetweenDeliveries) noexcept
{
    dbg(COLOR_BG_BLUE
        << "GraphQLConnectionOperation " << this << " id=" << m_id << ": intervalBetweenDeliveries="
        << std::chrono::duration_cast<std::chrono::milliseconds>(intervalBetweenDeliveries).count()
        << "ms");

    DLT_LOG(dltOperation,
        DLT_LOG_DEBUG,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": intervalBetweenDeliveries="),
        DLT_UINT64(std::chrono::duration_cast<std::chrono::milliseconds>(intervalBetweenDeliveries)
                       .count()),
        DLT_CSTRING("ms"));

    m_intervalBetweenDeliveries = intervalBetweenDeliveries;
}

void GraphQLConnectionOperationSubscription::subscribeInAThread() noexcept
{
    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << " key=" << m_subscriptionKey << ": already stopped, ignore subscribe");
        return;
    }

    auto ast = peg::parseString(m_query);

    auto spThis = getSharedPtr();
    m_subscriptionKey = m_executableSchema.subscribe(
        service::SubscriptionParams { spThis, ast, m_operationName, std::move(m_variables) },
        std::bind(&GraphQLConnectionOperationSubscription::onFutureSubscription,
            spThis,
            std::placeholders::_1));

    m_subscriptionName = getSubscriptionName(ast);
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << ": subscribed as key=" << m_subscriptionKey
                      << " name=" << m_subscriptionName);

    DLT_LOG(dltOperation,
        DLT_LOG_DEBUG,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": subscribed key="),
        DLT_UINT64(m_subscriptionKey));

    // Do an initial delivery only to this subscription
    notify();
}

void GraphQLConnectionOperationSubscription::stop() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << ": stop key=" << m_subscriptionKey);

    CONNECTION_OPERATION_CHECK_MAIN_THREAD;

    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << " key=" << m_subscriptionKey
                          << ": already stopped, ignore another stop command!");
        return;
    }

    m_stopped = true;

    if (m_deliveryTimer)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << " key=" << m_subscriptionKey
                          << ": unsubscribed with pending deliveries, ignore them");
        m_deliveryTimer.reset();
    }
    m_pendingDelivery = {};

    boost::asio::defer(m_thread,
        std::bind(&GraphQLConnectionOperationSubscription::unsubscribeInAThread, getSharedPtr()));
    m_thread.join();
}

void GraphQLConnectionOperationSubscription::unsubscribeInAThread() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << ": unsubscribe key=" << m_subscriptionKey);

    std::lock_guard<SpinLock> guard(m_scopedSignalConnectionsLock);
    m_scopedSignalConnections.clear();

    m_executableSchema.unsubscribe(m_subscriptionKey);

    DLT_LOG(dltOperation,
        DLT_LOG_DEBUG,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(": unsubscribed key="),
        DLT_UINT64(m_subscriptionKey));
}

void GraphQLConnectionOperationSubscription::onFutureSubscription(
    std::future<response::Value> futureResponse) noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << " key=" << m_subscriptionKey << ": got future subscription response");

    CONNECTION_OPERATION_CHECK_MAIN_THREAD;

    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << " key=" << m_subscriptionKey
                          << ": already stopped, ignore subscription resolution");
        return;
    }

    if (!shouldDispatchCurrentDelivery())
        return;

    auto timeRemainingToDeliver = calculateTimeRemainingToDeliver();
    m_pendingDelivery = std::move(futureResponse);

    if (m_deliveryTimer)
        return; // already scheduled, don't reschedule it (otherwise it may never expire)

    m_deliveryTimer = m_handlers.createTimer();
    m_deliveryTimer->expires_after(timeRemainingToDeliver);
    m_deliveryTimer->async_wait([spThis = getSharedPtr()](const boost::system::error_code& error) {
        if (error)
        {
            dbg(COLOR_BG_BLUE "GraphQLConnectionOperation "
                << spThis.get() << " id=" << spThis->m_id << " key=" << spThis->m_subscriptionKey
                << ": timer error=" << error.message());
            return;
        }
        spThis->dispatchPendingDelivery();
    });
}

service::SubscriptionName
GraphQLConnectionOperationSubscription::getSubscriptionName(const peg::ast& ast) const noexcept
{
    FragmentDefinitionVisitor fragmentVisitor(m_variables);
    peg::for_each_child<peg::fragment_definition>(*ast.root,
        [&fragmentVisitor](const peg::ast_node& child) {
            fragmentVisitor.visit(child);
        });

    SubscriptionDefinitionNameVisitor subscriptionVisitor(fragmentVisitor.getFragments());
    peg::for_each_child<peg::operation_definition>(*ast.root,
        [&subscriptionVisitor](const peg::ast_node& child) {
            subscriptionVisitor.visit(child);
        });

    return subscriptionVisitor.getName();
}

bool GraphQLConnectionOperationSubscription::shouldDispatchCurrentDelivery() const noexcept
{
    const auto& triggers = m_handlers.currentNotificationTriggers();

    return triggers.hasSubscriptionKey(m_subscriptionKey);
}

std::chrono::steady_clock::duration
GraphQLConnectionOperationSubscription::calculateTimeRemainingToDeliver() const noexcept
{
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastDelivery = now - m_lastDelivery;
    auto timeRemainingToDeliver = std::max(std::chrono::steady_clock::duration(0),
        m_intervalBetweenDeliveries - timeSinceLastDelivery);
    if (timeRemainingToDeliver > std::chrono::steady_clock::duration(0))
        dbg(COLOR_BG_BLUE
            << "GraphQLConnectionOperation " << this << " id=" << m_id
            << " key=" << m_subscriptionKey << ": rate limited, deliver in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(timeRemainingToDeliver).count()
            << "ms");

    return timeRemainingToDeliver;
}

void GraphQLConnectionOperationSubscription::dispatchPendingDelivery() noexcept
{
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << " key=" << m_subscriptionKey << ": dispatch pending delivery");

    CONNECTION_OPERATION_CHECK_MAIN_THREAD;

    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id << " key="
                          << m_subscriptionKey << ": already stopped, ignore pending delivery");
        return;
    }

    m_lastDelivery = std::chrono::steady_clock::now();
    m_deliveryTimer.reset();

    boost::asio::defer(m_thread,
        std::bind(&GraphQLConnectionOperationSubscription::resolveSubscriptionInAThread,
            getSharedPtr(),
            // NOTE: we can't bind directly to std::future<> since it's not copyable and std::bind()
            // doesn't know it will be called only once. Then use a shared_ptr to allow it to pass
            // the checks, but it will be used by exactly one holder
            std::make_shared<std::future<response::Value>>(std::move(m_pendingDelivery))));
}

void GraphQLConnectionOperationSubscription::resolveSubscriptionInAThread(
    std::shared_ptr<std::future<response::Value>> futureResponsePtr) noexcept
{
    auto onReply = m_handlers.onReply; // copy before checking m_stopped to avoid race
    auto futureResponse = std::move(*futureResponsePtr);
    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << " key=" << m_subscriptionKey
                          << ": already stopped, ignore threaded subscription resolution");
        return;
    }

    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id << " key="
                      << m_subscriptionKey << ": resolving subscription response in a thread");
    DLT_LOG(dltOperation,
        DLT_LOG_VERBOSE,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(" key="),
        DLT_UINT64(m_subscriptionKey),
        DLT_CSTRING(": resolve subscription in a thread"));

#ifdef GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
    auto startTime = std::chrono::high_resolution_clock::now();
#endif

    auto response = futureResponse.get();

#ifdef GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = endTime - startTime;
#else
    std::chrono::milliseconds elapsed(0);
#endif

    if (m_stopped)
    {
        dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                          << ": already stopped, ignore reply");
        return;
    }
    onReply(response::helpers::createResponse(GQL_DATA, m_id, std::move(response)));

    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id
                      << " key=" << m_subscriptionKey << ": resolved [stats: elapsed="
                      << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                      << "ms, singletons=" << m_usedSingletons.size() << "]");

    if (!m_failedPermissionsCheck)
        m_didPermissionsCheck = true;
    else
    {
        dbg(COLOR_BG_RED << "GraphQLConnectionOperation " << this << " id=" << m_id << " key="
                         << m_subscriptionKey << ": failed permissions check, force stop");
        DLT_LOG(dltOperation,
            DLT_LOG_WARN,
            DLT_CSTRING("operation="),
            DLT_PTR(this),
            DLT_CSTRING(" id="),
            DLT_SIZED_STRING(m_id.data(), m_id.size()),
            DLT_CSTRING(" key="),
            DLT_UINT64(m_subscriptionKey),
            DLT_CSTRING(":failed permissions check"));
        m_handlers.defer(std::bind(&GraphQLConnectionOperationSubscription::stop, getSharedPtr()));
    }
}

void GraphQLConnectionOperationSubscription::addScopedSignalConnection(
    boost::signals2::scoped_connection&& con) noexcept
{
    std::lock_guard<SpinLock> guard(m_scopedSignalConnectionsLock);
    m_scopedSignalConnections.push_back(std::move(con));
    dbg(COLOR_BG_BLUE << "GraphQLConnectionOperation " << this << " id=" << m_id << " key="
                      << m_subscriptionKey << ": observe " << &m_scopedSignalConnections.back());
}

void GraphQLConnectionOperationSubscription::notify() noexcept
{
    DLT_LOG(dltOperation,
        DLT_LOG_VERBOSE,
        DLT_CSTRING("operation="),
        DLT_PTR(this),
        DLT_CSTRING(" id="),
        DLT_SIZED_STRING(m_id.data(), m_id.size()),
        DLT_CSTRING(" key="),
        DLT_UINT64(m_subscriptionKey),
        DLT_CSTRING(": notify"));
    m_handlers.notify(std::make_shared<GraphQLNotifyTriggers>(
        graphql::service::SubscriptionName { m_subscriptionName },
        std::set<graphql::service::SubscriptionKey> { m_subscriptionKey }));
}
