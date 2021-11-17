// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#pragma once

#include <graphqlservice/GraphQLService.h>

#include <chrono>

#include "graphqlrequeststate.hpp"
#include "graphql_vss_server_libs-protocol_export.h"

class GraphQLConnectionOperation : public GraphQLRequestState
{
public:
    static std::shared_ptr<GraphQLConnectionOperation>
    make(const std::string_view& id, const GraphQLRequestHandlers& handlers,
        service::Request& executableSchema, std::shared_ptr<const ClientPermissions> permissions,
        SingletonStorage& singletonStorage, response::Value&& payload);

    GraphQLConnectionOperation(const std::string_view& id, const GraphQLRequestHandlers& handlers,
        service::Request& executableSchema, std::shared_ptr<const ClientPermissions> permissions,
        SingletonStorage& singletonStorage, bool isSubscription, std::string&& query,
        std::string&& operationName, response::Value&& variables);
    virtual ~GraphQLConnectionOperation();

    GraphQLConnectionOperation(GraphQLConnectionOperation const&) = delete;
    GraphQLConnectionOperation(GraphQLConnectionOperation&&) = delete;

    inline const std::string_view getId() const
    {
        return m_id;
    }

    constexpr bool isStopped() noexcept
    {
        return m_stopped;
    }

    virtual void start() noexcept = 0;
    virtual void stop() noexcept = 0;

protected:
    const std::string m_id;
    bool m_stopped;

    const std::string m_query; // keep alive as queryAst may point to it
    const std::string m_operationName;
    response::Value m_variables;

#if GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
    const std::thread::id m_threadId = std::this_thread::get_id();
    void checkThread(const std::string_view& fn) const;
#endif
};
