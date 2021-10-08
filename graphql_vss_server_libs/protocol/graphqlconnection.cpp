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

#include <graphql_vss_server_libs/support/debug.hpp>
#include <graphql_vss_server_libs/support/log.hpp>

#include "authorizer.hpp"
#include "exceptions.hpp"
#include "messagetypes.hpp"
#include "response_helpers.hpp"

#include "graphqlconnection.hpp"

// Added to backward compatibility with older versions of DLT Daemon
#include "dlt_helpers.hpp"

GraphQLConnection::~GraphQLConnection()
{
	dbg(COLOR_BG_GREEN << "~GraphQLConnection " << this << " operations=" << m_operations.size());
	for (auto& it : m_operations)
		it.second->stop();
}

void GraphQLConnection::setup(Authorizer* authorizer, service::Request* executableSchema,
	SingletonStorage* singletonStorage, GraphQLRequestHandlers&& handlers)
{
	dbg(COLOR_BG_GREEN << "GraphQLConnection setup " << this);

	m_authorizer = authorizer;
	m_executableSchema = executableSchema;
	m_singletonStorage = singletonStorage;
	// handlers are bound to the server, thus we introduce a ref-cycle, tearDown() fixes it
	m_handlers = std::move(handlers);
}

void GraphQLConnection::tearDown()
{
	DLT_LOG(dltConnection,
		DLT_LOG_INFO,
		DLT_CSTRING("connection="),
		DLT_PTR(this),
		DLT_CSTRING("tearDown, operations="),
		DLT_UINT64(m_operations.size()));
	dbg(COLOR_BG_GREEN << "GraphQLConnection tearDown " << this
					   << " operations=" << m_operations.size());

	stop();

	m_permissions = nullptr;

	m_authorizer = nullptr;
	m_executableSchema = nullptr;
	m_singletonStorage = nullptr;
	// forceful release resources, specially the following as they contain ref-cycles
	m_handlers = {};
}

void GraphQLConnection::stop()
{
	DLT_LOG(dltConnection,
		DLT_LOG_DEBUG,
		DLT_CSTRING("connection="),
		DLT_PTR(this),
		DLT_CSTRING("stop, operations="),
		DLT_UINT64(m_operations.size()));
	dbg(COLOR_BG_GREEN << "GraphQLConnection stop " << this
					   << " operations=" << m_operations.size());

	auto operations = std::move(m_operations);
	for (auto& it : operations)
	{
		if (!it.second->isStopped())
			it.second->stop();
	}
	operations.clear();
}

void GraphQLConnection::onHttp(const std::string& authorization, response::Value&& payload) noexcept
{
	std::string_view id = "0";

	try
	{
		DLT_LOG(dltConnection,
			DLT_LOG_DEBUG,
			DLT_CSTRING("connection="),
			DLT_PTR(this),
			DLT_CSTRING("http, authorization="),
			DLT_SIZED_UTF8(authorization.data(), authorization.size()));
		m_permissions = m_authorizer->authorize(authorization);
		onGraphQLMessage(GQL_START, id, std::move(payload));
	}
	catch (std::exception& ex)
	{
		reply(response::helpers::createErrorResponse(GQL_CONNECTION_ERROR, id, ex));
	}
}

void GraphQLConnection::onWebSocketMessage(response::Value&& message) noexcept
{
	std::string id;

	try
	{
		auto [type, mId, payload] = response::helpers::toMessageParts(std::move(message));
		id = std::move(mId);

		onGraphQLMessage(type, id, std::move(payload));
	}
	catch (std::exception& ex)
	{
		const auto errorType = id.empty() ? GQL_CONNECTION_ERROR : GQL_ERROR;
		DLT_LOG(dltConnection,
			DLT_LOG_WARN,
			DLT_CSTRING("connection="),
			DLT_PTR(this),
			DLT_SIZED_CSTRING(errorType.data(), errorType.size()),
			DLT_STRING(ex.what()));
		reply(response::helpers::createErrorResponse(errorType, id, ex));
	}
}

void GraphQLConnection::onGraphQLMessage(
	const std::string_view& type, const std::string_view& id, response::Value&& payload)
{
	// https://github.com/apollographql/subscriptions-transport-ws/blob/master/PROTOCOL.md
	if (type == GQL_CONNECTION_INIT)
		onGraphQLConnectionInit(std::move(payload));
	else if (type == GQL_CONNECTION_TERMINATE)
		onGraphQLConnectionTerminate();
	else if (type == GQL_START)
		onGraphQLStart(id, std::move(payload));
	else if (type == GQL_STOP)
		onGraphQLStop(id);
	else
	{
		std::string msg("Invalid message type: ");
		throw InvalidPayload(msg + std::string { type });
	}
}

void GraphQLConnection::onGraphQLConnectionInit(response::Value&& payload)
{
	if (payload.type() != response::Type::Map)
		throw InvalidPayload("Connection init payload is not an object");

	for (auto& m : payload.release<response::MapType>())
	{
		if (m.first == GQL_AUTHORIZATION)
		{
			DLT_LOG(dltConnection,
				DLT_LOG_DEBUG,
				DLT_CSTRING("connection="),
				DLT_PTR(this),
				DLT_CSTRING("ws, authorization="),
				DLT_SIZED_UTF8(m.second.get<response::StringType>().data(),
					m.second.get<response::StringType>().size()));
			m_permissions = m_authorizer->authorize(m.second.release<response::StringType>());
			break;
		}
	}

	reply(response::helpers::createResponse(GQL_CONNECTION_ACK, "", response::Value()));
}

void GraphQLConnection::onGraphQLConnectionTerminate()
{
	if (!m_handlers.terminate)
	{
		dbg(COLOR_BG_BLUE << "GraphQLConnection " << this
						  << " already torn down, ignore terminate");
		return;
	}
	m_handlers.terminate();
}

void GraphQLConnection::onGraphQLStart(const std::string_view& id, response::Value&& payload)
{
	if (id.empty())
		throw InvalidPayload("missing 'id'");

	auto itr = m_operations.find(id);
	if (itr != m_operations.end())
	{
		dbg(COLOR_BG_BLUE << "GraphQLConnection start: ignored duplicated start id=" << id);
		return;
	}

	addConnectionOperation(id, std::move(payload));
}

void GraphQLConnection::onGraphQLStop(const std::string_view& id)
{
	auto itr = m_operations.find(id);
	if (itr == m_operations.end())
	{
		dbg(COLOR_BG_BLUE << "GraphQLConnection stop: ignored unknown operation id=" << id);
		return;
	}

	DLT_LOG(dltConnection,
		DLT_LOG_DEBUG,
		DLT_CSTRING("connection="),
		DLT_PTR(this),
		DLT_CSTRING("stop operation="),
		DLT_PTR(itr->second.get()),
		DLT_SIZED_STRING(itr->second->getId().data(), itr->second->getId().size()));

	itr->second->stop();
	m_operations.erase(itr);
}

std::shared_ptr<GraphQLConnectionOperation>
GraphQLConnection::addConnectionOperation(const std::string_view& id, response::Value&& payload)
{
	auto op = GraphQLConnectionOperation::make(id,
		m_handlers,
		*m_executableSchema,
		m_permissions,
		*m_singletonStorage,
		std::move(payload));

	m_operations.insert({ op->getId(), op });
	DLT_LOG(dltConnection,
		DLT_LOG_DEBUG,
		DLT_CSTRING("connection="),
		DLT_PTR(this),
		DLT_CSTRING("add operation="),
		DLT_PTR(op.get()),
		DLT_SIZED_STRING(op->getId().data(), op->getId().size()));
	op->start();

	return op;
}

void GraphQLConnection::reply(response::Value&& response) noexcept
{
	if (!m_handlers.onReply)
	{
		dbg(COLOR_BG_BLUE << "GraphQLConnection " << this << " already torn down, ignore reply");
		return;
	}
	m_handlers.onReply(std::move(response));
}
