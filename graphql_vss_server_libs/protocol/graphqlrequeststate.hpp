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

#include <boost/asio.hpp>

#undef NO_DATA // Removes definition on netdb.h that may conflict enum items
               // of CommonAPI generated files. This is included by
               // <boost/asio.hpp>, <boost/asio/steady_timer.hpp> and
               // <boost/asio/signal_set.hpp>

#include <graphqlservice/GraphQLService.h>
#include <boost/signals2.hpp>

#include <graphql_vss_server_libs/support/permissions.hpp>
#include <graphql_vss_server_libs/support/singleton.hpp>

#include "graphqlrequesthandlers.hpp"

#include "exceptions.hpp"

using namespace graphql;

class GraphQLRequestState : public service::RequestState
{
public:
    GraphQLRequestState(const GraphQLRequestHandlers& handlers, service::Request& executableSchema,
        std::shared_ptr<const ClientPermissions> permissions, SingletonStorage& singletonStorage,
        bool isSubscription);

    GraphQLRequestState(GraphQLRequestState const&) = delete;
    GraphQLRequestState(GraphQLRequestState&&) = delete;

    static inline std::shared_ptr<GraphQLRequestState>
    fromRequestState(const std::shared_ptr<RequestState>& requestState)
    {
        return std::static_pointer_cast<GraphQLRequestState>(requestState);
    }

    template <typename... T>
    inline void validate(const T&... requiredPermissions)
    {
        if (m_didPermissionsCheck)
            return;
        if (!m_permissions)
        {
            m_failedPermissionsCheck = true;
            throw ContextException();
        }
        try
        {
            m_permissions->validate(requiredPermissions...);
        }
        catch (PermissionException& ex)
        {
            m_failedPermissionsCheck = true;
            throw;
        }
    }

    template <typename TSignal>
    inline void observe(boost::signals2::signal<void(TSignal)>& signal)
    {
        if (!m_isSubscription)
            return;

        addScopedSignalConnection(signal.connect([this](TSignal) {
            this->notify();
        }));
    }

    template <typename TSingletonValue>
    inline std::shared_ptr<TSingletonValue> getSingleton()
    {
        auto key = Singleton<TSingletonValue>::getKey();

        std::unique_lock lock(m_usedSingletonsLock);
        auto itr = m_usedSingletons.find(key);
        if (itr != m_usedSingletons.end())
        {
            auto ref = typename Singleton<TSingletonValue>::Ref(itr->second);
            lock.unlock();
            return ref.value();
        }

        auto ref = m_singletonStorage.get<TSingletonValue>();
        m_usedSingletons.insert({ key, ref.base() });
        lock.unlock();

        auto singleton = ref.value();

        if constexpr (has_signal<TSingletonValue>::value)
            observe(singleton->signal);

        return singleton;
    }

    virtual void
    setSubscriptionmIntervalBetweenDeliveries(std::chrono::milliseconds intervalInMs) noexcept
    {
    }

protected:
    const GraphQLRequestHandlers& m_handlers;
    service::Request& m_executableSchema;
    const std::shared_ptr<const ClientPermissions> m_permissions;
    SingletonStorage& m_singletonStorage;
    const bool m_isSubscription;
    bool m_didPermissionsCheck;
    bool m_failedPermissionsCheck = false;
    SpinLock m_usedSingletonsLock = SpinLock(this);
    std::map<BaseSingleton::Key, BaseSingleton::Ref> m_usedSingletons;

    virtual void addScopedSignalConnection(boost::signals2::scoped_connection&& con) noexcept
    {
    }

    virtual void notify() noexcept
    {
    }

    template <class T>
    class has_signal
    {
        template <class U>
        static char test(decltype(&U::signal));
        template <class U>
        static int test(...);

    public:
        static constexpr bool value = sizeof(test<T>(nullptr)) == sizeof(char);
    };
};
