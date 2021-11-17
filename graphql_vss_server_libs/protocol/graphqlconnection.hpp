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

#include "graphqlconnectionoperation.hpp"

using namespace graphql;

class ClientPermissions;
class Authorizer;

class GraphQLConnection
{
public:
    GraphQLConnection() = default;
    GraphQLConnection(GraphQLConnection const&) = delete;
    GraphQLConnection(GraphQLConnection&&) = delete;
    ~GraphQLConnection();

    // The connection is constructed by websocket::server and we can't pass extra parameters, then
    // we need this
    void setup(Authorizer* authorizer, service::Request* executableSchema,
        SingletonStorage* singletonStorage, GraphQLRequestHandlers&& handlers);

    // Release any references, in particular the circular ones
    void tearDown();

    void stop();

    // Requests following https://graphql.org/learn/serving-over-http/
    // Assumes connection setup with deferred HTTP response
    void onHttp(const std::string& authorization, response::Value&& payload) noexcept;

    // Messages following
    // https://github.com/apollographql/subscriptions-transport-ws/blob/master/PROTOCOL.md
    void onWebSocketMessage(response::Value&& message) noexcept;

private:
    Authorizer* m_authorizer;
    service::Request* m_executableSchema;
    SingletonStorage* m_singletonStorage;
    GraphQLRequestHandlers m_handlers;

    std::shared_ptr<const ClientPermissions> m_permissions;
    std::map<std::string_view, std::shared_ptr<GraphQLConnectionOperation>> m_operations;

protected:
    void onGraphQLMessage(
        const std::string_view& type, const std::string_view& id, response::Value&& payload);
    void onGraphQLConnectionInit(response::Value&& payload);
    void onGraphQLConnectionTerminate();
    void onGraphQLStart(const std::string_view& id, response::Value&& payload);
    void onGraphQLStop(const std::string_view& id);

    std::shared_ptr<GraphQLConnectionOperation>
    addConnectionOperation(const std::string_view& id, response::Value&& payload);
    void reply(response::Value&& response) noexcept;
};
