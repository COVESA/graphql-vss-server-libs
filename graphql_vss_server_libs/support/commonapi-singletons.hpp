// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi),
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#pragma once
#include <graphqlservice/GraphQLService.h>
#include <CommonAPI/CommonAPI.hpp>
#include <boost/signals2.hpp>
#include <optional>

#include "demangle.hpp"
#include "singleton.hpp"
#include "spinlock.hpp"
#include "type_traits_extras.hpp"
#include "log.hpp"
#include "dlt_helpers.hpp"

#include "graphql_vss_server_libs-support_export.h"

#define SUBSCRIPTION_TIMEOUT_SECONDS 5

using namespace graphql;

extern GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::shared_ptr<CommonAPI::Runtime>
    commonAPISingletonProxyRuntime;
extern GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::string commonAPISingletonProxyConnectionId;
extern GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::string commonAPISingletonProxyDomain;

template <typename AttributeGetterPointer>
struct CommonAPIProxyAttributeGetterTraits
{
    using AttributeGetter = pointer_member_traits<AttributeGetterPointer>;
    using Proxy = typename AttributeGetter::container_type;
    using Attribute = typename std::remove_reference<
        typename function_signature<typename AttributeGetter::member_type>::return_type>::type;
};

template <typename AttributeGetterPointer>
struct CommonAPIProxyAttributeGetterValueTraits
    : public CommonAPIProxyAttributeGetterTraits<AttributeGetterPointer>
{
    using typename CommonAPIProxyAttributeGetterTraits<AttributeGetterPointer>::AttributeGetter;
    using typename CommonAPIProxyAttributeGetterTraits<AttributeGetterPointer>::Proxy;
    using typename CommonAPIProxyAttributeGetterTraits<AttributeGetterPointer>::Attribute;
    using AttributeGetValue =
        typename pointer_member_traits<decltype(&Attribute::getValue)>::member_type;
    using AttributeValue = typename std::remove_reference<std::tuple_element_t<1,
        typename function_signature<AttributeGetValue>::arguments_types>>::type;
};

template <typename... T>
struct CommonAPIEventHandlerTraits;

template <typename... T>
struct CommonAPIEventHandlerTraits<std::tuple<T...>>
{
    typedef std::tuple<const T&...> Tuple;
    typedef std::function<void(const T&... args)> Type;

    static Type create(std::function<void(Tuple&&)>&& handler)
    {
        return Type([handler](const T&... args) {
            handler(std::make_tuple<const T&...>(args...));
        });
    }
};

template <typename E>
struct CommonAPIEventTraits
{
    typedef CommonAPIEventHandlerTraits<typename E::ArgumentsTuple> Handler;
};

template <template <typename...> class ProxyClass_, const std::string_view& instanceId>
struct CommonAPIProxy
{
    typedef ProxyClass_<> Proxy;
    typedef CommonAPIProxy<ProxyClass_, instanceId> Self;

    std::shared_ptr<Proxy> proxy;

    static std::shared_ptr<Self> create()
    {
        std::shared_ptr<Proxy> proxy =
            commonAPISingletonProxyRuntime->buildProxy<ProxyClass_>(commonAPISingletonProxyDomain,
                std::string(instanceId),
                commonAPISingletonProxyConnectionId);

        if (proxy == nullptr)
        {
            std::ostringstream msg;
            msg << "ERROR: Problem while creating proxy '" << demangle<Proxy>() << "', instance: '"
                << instanceId << "', domain: '" << commonAPISingletonProxyDomain
                << "', connectionId: '" << commonAPISingletonProxyConnectionId << "'";
#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
            msg << ", symbol: " << demangle<Proxy>();
#endif
            DLT_LOG(dltCommonAPI,
                DLT_LOG_ERROR,
                DLT_UTF8(msg.str().c_str()));
            throw std::runtime_error(msg.str());
        }

        for (size_t retries = 0; retries < 1e5 && !proxy->isAvailable(); retries++)
            std::this_thread::sleep_for(std::chrono::microseconds(5));

        if (!proxy->isAvailable())
        {
            std::ostringstream msg;
            msg << "ERROR: Proxy couldn't be available: proxy '" << demangle<Proxy>()
                << "', instance: '" << instanceId << "', domain: '" << commonAPISingletonProxyDomain
                << "', connectionId: '" << commonAPISingletonProxyConnectionId << "'";
#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
            msg << ", symbol: " << demangle<Proxy>();
#endif
            DLT_LOG(dltCommonAPI,
                    DLT_LOG_ERROR,
                    DLT_UTF8(msg.str().c_str()));
            throw std::runtime_error(msg.str());
        }

        return std::make_shared<Self>(std::move(proxy));
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage*)
    {
        return std::async(std::launch::async, create);
    }

    CommonAPIProxy(std::shared_ptr<Proxy>&& proxy)
        : proxy(std::move(proxy))
    {
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Proxy "),
            DLT_SIZED_UTF8(demangle<Proxy>().data(), demangle<Proxy>().size()),
            DLT_CSTRING(" "),
            DLT_SIZED_UTF8(instanceId.data(), instanceId.size()),
            DLT_CSTRING(" created "),
            DLT_PTR(this));
        dbg(COLOR_BG_BLUE << "Proxy " << demangle<Proxy>() << " " << instanceId << " created "
                          << this);
    }

    CommonAPIProxy(CommonAPIProxy const& other) = delete; // avoid copies, it's an usage bug!
    CommonAPIProxy(CommonAPIProxy&& other) = delete;      // avoid copies, it's an usage bug!

    ~CommonAPIProxy()
    {
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Proxy "),
            DLT_SIZED_UTF8(demangle<Proxy>().data(), demangle<Proxy>().size()),
            DLT_CSTRING(" "),
            DLT_SIZED_UTF8(instanceId.data(), instanceId.size()),
            DLT_CSTRING(" destroyed "),
            DLT_PTR(this));
        dbg(COLOR_BG_BLUE << "Proxy " << demangle<Proxy>() << " " << instanceId << " destroyed "
                          << this);
        proxy.reset();
    }
};

template <typename TValue, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPIBaseProxyAttribute
{
public:
    typedef TProxy Proxy;
    typedef TAttribute Attribute;
    typedef TValue Value;
    typedef CommonAPIBaseProxyAttribute<Value, Proxy, Attribute, TGetAttribute> Self;

    explicit CommonAPIBaseProxyAttribute(typename Singleton<Proxy>::Ref&& proxySingleton)
        : m_proxySingleton(std::move(proxySingleton))
        , m_attribute((m_proxySingleton.value()->proxy.get()->*TGetAttribute)())
    {
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Attribute "),
            DLT_SIZED_UTF8(demangle<Proxy>().data(), demangle<Proxy>().size()),
            DLT_CSTRING(" "),
            DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size()),
            DLT_CSTRING(" created "),
            DLT_PTR(this));
        dbg(COLOR_BG_CYAN << "Attribute " << demangle<Proxy>() << " " << demangle<Attribute>()
                          << " created " << this);
    }

    CommonAPIBaseProxyAttribute(CommonAPIBaseProxyAttribute const& other) = delete;
    CommonAPIBaseProxyAttribute(CommonAPIBaseProxyAttribute&& other) = delete;

    ~CommonAPIBaseProxyAttribute()
    {
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Attribute "),
            DLT_SIZED_UTF8(demangle<Proxy>().data(), demangle<Proxy>().size()),
            DLT_CSTRING(" "),
            DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size()),
            DLT_CSTRING(" destroyed "),
            DLT_PTR(this));
        dbg(COLOR_BG_CYAN << "Attribute " << demangle<Proxy>() << " " << demangle<Attribute>()
                          << " destroyed " << this);
    }

protected:
    typename Singleton<Proxy>::Ref m_proxySingleton;
    Attribute& m_attribute;

    Value fetchValue() const
    {
        Value response;
        CommonAPI::CallStatus status;

        this->m_attribute.getValue(status, response);
        if (status != CommonAPI::CallStatus::SUCCESS)
        {
            std::ostringstream error_message;
            error_message << "Error while fetching for value: " << demangle<Self>();
            DLT_LOG(dltCommonAPI,
                DLT_LOG_ERROR,
                DLT_UTF8(error_message.str().c_str()));
            throw std::runtime_error(error_message.str());
        }
        return response;
    }

    Value changeValue(const Value& input) const
    {
        Value response;
        CommonAPI::CallStatus status;

        this->m_attribute.setValue(input, status, response);
        if (status != CommonAPI::CallStatus::SUCCESS)
        {
            std::ostringstream error_message;
            error_message << "Error while changing value: " << demangle<Self>();
            DLT_LOG(dltCommonAPI,
                DLT_LOG_ERROR,
                DLT_UTF8(error_message.str().c_str()));
            throw std::runtime_error(error_message.str());
        }
        return response;
    }
};

template <typename ValueType, typename LockType>
class LockedValueReference
{
public:
    ValueType& value;

    explicit LockedValueReference(ValueType& _value, LockType& _lock)
        : value(_value)
        , lock(_lock)
    {
    }

    LockedValueReference(
        LockedValueReference const& other) = delete;             // avoid copies, it's an usage bug!
    LockedValueReference(LockedValueReference&& other) = delete; // avoid copies, it's an usage bug!

private:
    const std::lock_guard<LockType> lock;
};

// Get the value in getValue or refreshValueUnlocked and then just reuse it
// NOTE: no changes will ever be applied, to track changes use one of:
// - CommonAPISubscriptionProxyAttribute
// or call refreshValue() manually
template <typename TValue, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPICachedProxyAttribute
    : public CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>
{
public:
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Proxy;
    using
        typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Attribute;
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Value;
    typedef CommonAPICachedProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute> Self;

    SpinLock lock = SpinLock(this);
    boost::signals2::signal<void(const Value&)> signal;

    static std::shared_ptr<Self> create(SingletonStorage* storage)
    {
        return std::make_shared<Self>(storage->get<Proxy>());
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage* storage)
    {
        return std::async(std::launch::async, create, storage);
    }

    explicit CommonAPICachedProxyAttribute(typename Singleton<Proxy>::Ref&& proxySingleton)
        : CommonAPIBaseProxyAttribute<Value, Proxy, Attribute, TGetAttribute>(
            std::move(proxySingleton))
    {
    }

    CommonAPICachedProxyAttribute(CommonAPICachedProxyAttribute const& other) = delete;
    CommonAPICachedProxyAttribute(CommonAPICachedProxyAttribute&& other) = delete;

    // This will lock internally!
    const Value& refreshValue()
    {
        return this->setValue(this->fetchValue());
    }

    const Value& refreshValueUnlocked()
    {
        this->m_value = this->fetchValue();
        signal(this->m_value);
        return this->m_value;
    }

    inline LockedValueReference<const Value, SpinLock> getValue()
    {
        return LockedValueReference<const Value, SpinLock>(this->m_value, this->lock);
    }

    template <typename TConverted>
    inline TConverted getValue(std::function<TConverted(const Value&)>&& convert)
    {
        std::lock_guard<SpinLock> guard(this->lock);
        return convert(this->m_value);
    }

    template <typename TConverted>
    inline TConverted getValue()
    {
        std::lock_guard<SpinLock> guard(this->lock);
        return TConverted(const_cast<const Value&>(this->m_value));
    }

    inline void mutateValue(const Value& input)
    {
        std::lock_guard<SpinLock> mutation_lock(lock);
        this->m_value = this->changeValue(input);
        signal(this->m_value);
    }

protected:
    Value m_value;

    const Value& setValue(const Value&& value)
    {
        std::unique_lock<SpinLock> guard(this->lock);
        this->m_value = std::move(value);
        signal(this->m_value);
        return this->m_value;
    }
};

// Always call CommonAPI to get a fresh value, it's never cached
template <typename TValue, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPIAlwaysGetValueProxyAttribute
    : public CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>
{
public:
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Proxy;
    using
        typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Attribute;
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Value;
    typedef CommonAPIAlwaysGetValueProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute> Self;

    static std::shared_ptr<Self> create(SingletonStorage* storage)
    {
        return std::make_shared<Self>(storage->get<Proxy>());
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage* storage)
    {
        return std::async(std::launch::deferred, create, storage);
    }

    using CommonAPIBaseProxyAttribute<Value, Proxy, Attribute,
        TGetAttribute>::CommonAPIBaseProxyAttribute; // reuse constructor
    CommonAPIAlwaysGetValueProxyAttribute(
        CommonAPIAlwaysGetValueProxyAttribute const& other) = delete;
    CommonAPIAlwaysGetValueProxyAttribute(CommonAPIAlwaysGetValueProxyAttribute&& other) = delete;

    // No need to lock, but it's a copy, not a reference!
    inline Value getValue() const
    {
        return this->fetchValue();
    }

    template <typename TConverted>
    inline TConverted getValue(std::function<TConverted(Value&&)>&& convert) const
    {
        return convert(this->getValue());
    }

    template <typename TConverted>
    inline TConverted getValue()
    {
        return TConverted(this->getValue());
    }

    inline void mutateValue(const Value& input)
    {
        this->changeValue(input);
    }
};

// Observe the value using getChangeEvent().subscribe()
template <typename TValue, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPISubscriptionOnlyProxyAttribute
    : public CommonAPICachedProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>
{
public:
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Proxy;
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Attribute;
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Value;
    typedef CommonAPISubscriptionOnlyProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute> Self;

    static std::shared_ptr<Self> create(SingletonStorage* storage)
    {
        return std::make_shared<Self>(storage->get<Proxy>());
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage* storage)
    {
        return std::async(std::launch::async, create, storage);
    }

    explicit CommonAPISubscriptionOnlyProxyAttribute(typename Singleton<Proxy>::Ref&& proxySingleton)
        : CommonAPICachedProxyAttribute<Value, Proxy, Attribute, TGetAttribute>(
            std::move(proxySingleton))
    {
        this->subscribeChanges();
    }

    CommonAPISubscriptionOnlyProxyAttribute(CommonAPISubscriptionOnlyProxyAttribute const& other) = delete;
    CommonAPISubscriptionOnlyProxyAttribute(CommonAPISubscriptionOnlyProxyAttribute&& other) = delete;

    ~CommonAPISubscriptionOnlyProxyAttribute()
    {
        std::unique_lock<SpinLock> guard(this->lock);
        this->unsubscribeChanges();
    }

protected:
    std::optional<typename CommonAPI::Event<Value>::Subscription> m_subscription;
    bool subscriptionAnswered;

    void subscribeChanges()
    {
        subscriptionAnswered = false;
        std::mutex m;
        std::condition_variable cv;

        std::thread t([this, &cv, &m]() {
            std::unique_lock<std::mutex> timerLock(m);
            if (cv.wait_for(timerLock, std::chrono::seconds(SUBSCRIPTION_TIMEOUT_SECONDS))
                == std::cv_status::timeout)
            {
                DLT_LOG(dltCommonAPI,
                    DLT_LOG_WARN,
                    DLT_CSTRING("Subscription timeout! Took too long to send the cached value "),
                    DLT_PTR(this));
                dbg(COLOR_BG_MAGENTA "Subscription timeout! Took too long to send the cached value " << this);
            }
        });

        this->m_subscription =
            this->m_attribute.getChangedEvent().subscribe([this, &cv](const Value& value) {
                if (!this->subscriptionAnswered)
                {
                    this->subscriptionAnswered = true;
                    cv.notify_one();
                }
                this->setValue(std::move(value));
            });

        t.join();

        if (!subscriptionAnswered)
        {
            this->unsubscribeChanges();
            std::ostringstream error_message;
            error_message << "Subscription timeout! "
                          << "The event subscribed had no cached value: " << demangle<Self>();
            DLT_LOG(dltCommonAPI,
                DLT_LOG_ERROR,
                DLT_UTF8(error_message.str().c_str()));
            throw std::runtime_error(error_message.str());
        }
    }

    void unsubscribeChanges()
    {
        if (this->m_subscription.has_value())
        {
            this->m_attribute.getChangedEvent().unsubscribe(this->m_subscription.value());
            this->m_subscription.reset();
        }
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Unsubscribing to changes: "),
            DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size())
        );
    }
};

// Fetch the initial value and then observe using getChangeEvent().subscribe()
template <typename TValue, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPISubscriptionProxyAttribute
    : public CommonAPISubscriptionOnlyProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>
{
public:
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Proxy;
    using
        typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Attribute;
    using typename CommonAPIBaseProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute>::Value;
    typedef CommonAPISubscriptionProxyAttribute<TValue, TProxy, TAttribute, TGetAttribute> Self;

    static std::shared_ptr<Self> create(SingletonStorage* storage)
    {
        return std::make_shared<Self>(storage->get<Proxy>());
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage* storage)
    {
        return std::async(std::launch::async, create, storage);
    }

    explicit CommonAPISubscriptionProxyAttribute(typename Singleton<Proxy>::Ref&& proxySingleton)
        : CommonAPISubscriptionOnlyProxyAttribute<Value, Proxy, Attribute, TGetAttribute>(
            std::move(proxySingleton))
    {
        this->refreshValueUnlocked();
    }
};

template <typename TProxy, typename TAttribute, TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPIEventSubscriptionProxyAttribute
{
public:
    typedef TProxy Proxy;
    typedef TAttribute Attribute;
    typedef CommonAPIEventSubscriptionProxyAttribute<Proxy, Attribute, TGetAttribute> Self;
    typedef typename CommonAPIEventTraits<Attribute>::Handler EventHandler;

    SpinLock lock = SpinLock(this);

    explicit CommonAPIEventSubscriptionProxyAttribute(
        typename Singleton<Proxy>::Ref&& proxySingleton)
        : m_proxySingleton(std::move(proxySingleton))
        , m_attribute((m_proxySingleton.value()->proxy.get()->*TGetAttribute)())
    {
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Attribute "),
            DLT_SIZED_UTF8(demangle<Proxy>().data(), demangle<Proxy>().size()),
            DLT_CSTRING(" "),
            DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size()),
            DLT_CSTRING(" created "),
            DLT_PTR(this));
        dbg(COLOR_BG_MAGENTA << "Attribute " << demangle<Proxy>() << " " << demangle<Attribute>()
                             << " created " << this);
        this->subscribeChanges();
    }

    CommonAPIEventSubscriptionProxyAttribute(
        CommonAPIEventSubscriptionProxyAttribute const& other) = delete;
    CommonAPIEventSubscriptionProxyAttribute(
        CommonAPIEventSubscriptionProxyAttribute&& other) = delete;

    virtual ~CommonAPIEventSubscriptionProxyAttribute()
    {
        this->unsubscribeChanges();
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Attribute "),
            DLT_SIZED_UTF8(demangle<Proxy>().data(), demangle<Proxy>().size()),
            DLT_CSTRING(" "),
            DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size()),
            DLT_CSTRING(" destroyed "),
            DLT_PTR(this));
        dbg(COLOR_BG_MAGENTA << "Attribute " << demangle<Proxy>() << " " << demangle<Attribute>()
                             << " destroyed " << this);
    }

protected:
    typename Singleton<Proxy>::Ref&& m_proxySingleton;
    Attribute& m_attribute;
    std::optional<typename Attribute::Subscription> m_subscription;

    virtual void processEvent(typename EventHandler::Tuple&& tuple) = 0;

    void subscribeChanges()
    {
        DLT_LOG(dltCommonAPI,
            DLT_LOG_INFO,
            DLT_CSTRING("Subscribing to changes"),
            DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size()));
        this->m_subscription =
            this->m_attribute.subscribe(EventHandler::create([this](auto&& args) {
                this->processEvent(std::move(args));
            }));
    }

    void unsubscribeChanges()
    {
        if (this->m_subscription.has_value())
        {
            DLT_LOG(dltCommonAPI,
                DLT_LOG_INFO,
                DLT_CSTRING("Unsubscribing to changes"),
                DLT_SIZED_UTF8(demangle<Attribute>().data(), demangle<Attribute>().size()));
            this->m_attribute.unsubscribe(this->m_subscription.value());
            this->m_subscription.reset();
        }
    }
};

template <typename TEvent, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPIBroadcastSubscriptionProxyAttribute
    : public CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute, TGetAttribute>
{
public:
    using
        typename CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute, TGetAttribute>::Proxy;
    using typename CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute,
        TGetAttribute>::Attribute;
    using typename CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute,
        TGetAttribute>::EventHandler;

    typedef TEvent Event;
    typedef std::shared_ptr<Event> Value;
    typedef CommonAPIBroadcastSubscriptionProxyAttribute<Event, Proxy, Attribute, TGetAttribute>
        Self;

    boost::signals2::signal<void(const Event&)> signal;

    static std::shared_ptr<Self> create(SingletonStorage* storage)
    {
        return std::make_shared<Self>(storage->get<Proxy>());
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage* storage)
    {
        return std::async(std::launch::deferred, create, storage);
    }

    using CommonAPIEventSubscriptionProxyAttribute<Proxy, Attribute,
        TGetAttribute>::CommonAPIEventSubscriptionProxyAttribute; // reuse constructor
    CommonAPIBroadcastSubscriptionProxyAttribute(
        CommonAPIBroadcastSubscriptionProxyAttribute const& other) = delete;
    CommonAPIBroadcastSubscriptionProxyAttribute(
        CommonAPIBroadcastSubscriptionProxyAttribute&& other) = delete;

    inline LockedValueReference<const Value, SpinLock> getValue()
    {
        return LockedValueReference<const Value, SpinLock>(this->m_value, this->lock);
    }

    template <typename TConverted>
    inline std::optional<TConverted> getValue(std::function<TConverted(const Event&)>&& convert)
    {
        std::lock_guard<SpinLock> guard(this->lock);
        if (!this->m_value)
            return std::nullopt;
        return std::make_optional<TConverted>(convert(*this->m_value));
    }

    template <typename TConverted>
    inline std::optional<TConverted> getValue()
    {
        std::lock_guard<SpinLock> guard(this->lock);
        if (!this->m_value)
            return std::nullopt;
        return std::optional<TConverted>(const_cast<const Event&>(*this->m_value));
    }

protected:
    Value m_value;

    void processEvent(typename EventHandler::Tuple&& tuple) override
    {
        std::unique_lock<SpinLock> guard(this->lock);
        m_value = std::make_shared<Event>(std::move(tuple));
        signal(*m_value);
    }
};

template <typename TEvent, typename TProxy, typename TAttribute,
    TAttribute& (TProxy::Proxy::*TGetAttribute)()>
class CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute
    : public CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute, TGetAttribute>
{
public:
    using
        typename CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute, TGetAttribute>::Proxy;
    using typename CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute,
        TGetAttribute>::Attribute;
    using typename CommonAPIEventSubscriptionProxyAttribute<TProxy, TAttribute,
        TGetAttribute>::EventHandler;

    typedef TEvent Event;
    typedef std::vector<std::shared_ptr<Event>> Value;
    typedef CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute<Event, Proxy, Attribute,
        TGetAttribute>
        Self;

    boost::signals2::signal<void(const Value&)> signal;

    static std::shared_ptr<Self> create(SingletonStorage* storage)
    {
        return std::make_shared<Self>(storage->get<Proxy>());
    }

    static std::future<std::shared_ptr<Self>> createFuture(SingletonStorage* storage)
    {
        return std::async(std::launch::deferred, create, storage);
    }

    using CommonAPIEventSubscriptionProxyAttribute<Proxy, Attribute,
        TGetAttribute>::CommonAPIEventSubscriptionProxyAttribute; // reuse constructor
    CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute(
        CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute const& other) = delete;
    CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute(
        CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute&& other) = delete;

    inline LockedValueReference<const Value, SpinLock> getValue()
    {
        return LockedValueReference<const Value, SpinLock>(this->m_value, this->lock);
    }

    template <typename TConverted>
    inline TConverted getValue(std::function<TConverted(const Value&)>&& convert)
    {
        std::lock_guard<SpinLock> guard(this->lock);
        return convert(this->m_value);
    }

    template <typename TConverted>
    inline TConverted getValue()
    {
        std::lock_guard<SpinLock> guard(this->lock);
        return TConverted(const_cast<const Value&>(this->m_value));
    }

protected:
    Value m_value;

    void processEvent(typename EventHandler::Tuple&& tuple) override
    {
        std::unique_lock<SpinLock> guard(this->lock);
        auto event = std::make_shared<Event>(std::move(tuple));
        auto it = std::find_if(m_value.begin(), m_value.end(), [event](const auto& other) -> bool {
            return Event::findPredicate(*event, *other);
        });
        if (it == m_value.end())
        {
            m_value.push_back(std::move(event));
        }
        else
        {
            *it = std::move(event);
        }
        signal(m_value);
    }
};

#define COMMONAPI_PROXY_ATTRIBUTE_TEMPLATE_ARGS(_proxy, _getter)                                   \
    _proxy, CommonAPIProxyAttributeGetterTraits<decltype(&_proxy::Proxy::_getter)>::Attribute,     \
        &_proxy::Proxy::_getter

#define COMMONAPI_PROXY_ATTRIBUTE_VALUE_TEMPLATE_ARGS(_proxy, _getter)                             \
    CommonAPIProxyAttributeGetterValueTraits<decltype(&_proxy::Proxy::_getter)>::AttributeValue,   \
        COMMONAPI_PROXY_ATTRIBUTE_TEMPLATE_ARGS(_proxy, _getter)

#define TYPEDEF_COMMONAPI_ALWAYS_GET_VALUE_PROXY_ATTRIBUTE(_proxy, _name)                          \
    typedef CommonAPIAlwaysGetValueProxyAttribute<                                                 \
        COMMONAPI_PROXY_ATTRIBUTE_VALUE_TEMPLATE_ARGS(_proxy, get##_name)>                         \
        _proxy##__##_name

#define TYPEDEF_COMMONAPI_SUBSCRIPTION_PROXY_ATTRIBUTE(_proxy, _name)                              \
    typedef CommonAPISubscriptionProxyAttribute<                                                   \
        COMMONAPI_PROXY_ATTRIBUTE_VALUE_TEMPLATE_ARGS(_proxy, get##_name)>                         \
        _proxy##__##_name

#define TYPEDEF_COMMONAPI_SUBSCRIPTION_ONLY_PROXY_ATTRIBUTE(_proxy, _name)                              \
    typedef CommonAPISubscriptionOnlyProxyAttribute<                                                   \
        COMMONAPI_PROXY_ATTRIBUTE_VALUE_TEMPLATE_ARGS(_proxy, get##_name)>                         \
        _proxy##__##_name

#define TYPEDEF_COMMONAPI_ACCUMULATIVE_UNIQUE_EVENT_SUBSCRIPTION_PROXY_ATTRIBUTE(_event,           \
    _proxy,                                                                                        \
    _name)                                                                                         \
    typedef CommonAPIAccumulativeUniqueEventSubscriptionProxyAttribute<_event,                     \
        COMMONAPI_PROXY_ATTRIBUTE_TEMPLATE_ARGS(_proxy, get##_name)>                               \
        _proxy##__##_name

#define TYPEDEF_COMMONAPI_EVENT_SUBSCRIPTION_PROXY_ATTRIBUTE(_event, _proxy, _name)                \
    typedef CommonAPIBroadcastSubscriptionProxyAttribute<_event,                                   \
        COMMONAPI_PROXY_ATTRIBUTE_TEMPLATE_ARGS(_proxy, get##_name)>                               \
        _proxy##__##_name

#define TYPEDEF_COMMONAPI_PROXY(_proxy, _instanceId, _name)                                        \
    static constexpr std::string_view _name##__instanceId(_instanceId);                            \
    typedef CommonAPIProxy<_proxy, _name##__instanceId> _name

// TODO:
// - add other typedef macros using COMMONAPI_PROXY_ATTRIBUTE_TEMPLATE_ARGS()
// - Broadcast
// - multi value events (seems not to exist with ObservableAttribute, but does in Broadcast.
// Others?)
