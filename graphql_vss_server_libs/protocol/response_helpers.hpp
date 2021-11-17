// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi)
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#pragma once

#include <graphqlservice/GraphQLResponse.h>
#include <websocketpp/http/constants.hpp>

#include <iostream>

#include <graphql_vss_server_libs/support/debug.hpp>

#include "exceptions.hpp"
#include "messagetypes.hpp"

namespace graphql {
namespace response {
namespace helpers {
[[maybe_unused]] static inline std::tuple<std::string, std::string, response::Value>
toMessageParts(response::Value&& message)
{
    if (message.type() != response::Type::Map)
        throw InvalidPayload("message is not an object");

    std::string id;
    std::string type;
    response::Value payload;

    for (auto& m : message.release<response::MapType>())
    {
        switch (m.second.type())
        {
            case response::Type::String:
                if (type.empty() && m.first == GQL_TYPE)
                    type = m.second.release<response::StringType>();
                else if (id.empty() && m.first == GQL_ID)
                    id = m.second.release<response::StringType>();
                break;

            case response::Type::Map:
                if (payload.type() == response::Type::Null && m.first == GQL_PAYLOAD)
                    payload = std::move(m.second);
                break;

            case response::Type::Null:
                // payload and others may come as null, silently ignore
                break;

            default:
                dbg(COLOR_BG_YELLOW << "Unexpected message item: " << m.first
                                    << ", type=" << (int)m.second.type());
        }
    }

    return std::make_tuple(std::move(type), std::move(id), std::move(payload));
}

[[maybe_unused]] static inline std::tuple<std::string, std::string, response::Value>
toOperationDefinitionParts(response::Value&& payload)
{
    if (payload.type() != response::Type::Map)
        throw InvalidPayload("start payload is not an object");

    std::string query;
    std::string operationName;
    response::Value variables;

    for (auto& m : payload.release<response::MapType>())
    {
        switch (m.second.type())
        {
            case response::Type::String:
                if (query.empty() && m.first == GQL_QUERY)
                    query = m.second.release<response::StringType>();
                else if (operationName.empty() && m.first == GQL_OPERATION_NAME)
                    operationName = m.second.release<response::StringType>();
                break;

            case response::Type::Map:
                if (variables.type() == response::Type::Null && m.first == GQL_VARIABLES)
                    variables = std::move(m.second);
                break;

            case response::Type::Null:
                // operationName and others may come as null, silently ignore
                break;

            default:
                dbg(COLOR_BG_YELLOW << "Unexpected operation definition item: " << m.first
                                    << ", type=" << (int)m.second.type());
        }
    }

    return std::make_tuple(std::move(query), std::move(operationName), std::move(variables));
}

static inline response::Value
createResponse(const std::string_view& type, const std::string_view& id, response::Value&& payload)
{
    response::Value response(response::Type::Map);
    response.emplace_back(GQL_TYPE.data(), response::Value(type.data()));

    if (!id.empty())
        response.emplace_back(GQL_ID.data(), response::Value(std::string { id }));

    if (payload.type() != response::Type::Null)
        response.emplace_back(GQL_PAYLOAD.data(), std::move(payload));

    return response;
}

[[maybe_unused]] static inline response::Value createErrorResponse(
    const std::string_view& type, const std::string_view& id, const std::exception& ex)
{
    static const std::array<std::pair<std::string_view, int>, 1> errorMapping = {
        // Prefix matching is bad, but other C++ solutions are not that good
        { { InvalidToken::messagePrefix, websocketpp::http::status_code::value::unauthorized } },
    };

    std::string_view message = ex.what();
    int statusCode = websocketpp::http::status_code::value::bad_request;
    for (const auto& pair : errorMapping)
    {
        const auto& prefix = pair.first;
        if (message.size() > prefix.size() && message.substr(0, prefix.size()) == prefix)
        {
            statusCode = pair.second;
            break;
        }
    }

    response::Value error(response::Type::Map);
    error.emplace_back(GQL_MESSAGE.data(), response::Value(std::string { message }));
    error.emplace_back(GQL_STATUS_CODE.data(), response::Value(statusCode));

    return createResponse(type, id, std::move(error));
}
} /* namespace helpers */
} /* namespace response */
} /* namespace graphql */
