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

#include "graphqlrequeststate.hpp"
#include "graphql_vss_server_libs-protocol_export.h"

GraphQLRequestState::GraphQLRequestState(const GraphQLRequestHandlers& handlers,
	service::Request& executableSchema, std::shared_ptr<const ClientPermissions> permissions,
	SingletonStorage& singletonStorage, bool isSubscription)
	: m_handlers(handlers)
	, m_executableSchema(executableSchema)
	, m_permissions(permissions)
	, m_singletonStorage(singletonStorage)
	, m_isSubscription(isSubscription)
	, m_didPermissionsCheck(DISABLE_PERMISSIONS)
{
#ifdef GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_DEBUG
	if (m_didPermissionsCheck)
	{
		static bool done = false;
		if (!done)
		{
			done = true;
			dbg(COLOR_BG_RED << "GraphQLRequestState: DISABLE_PERMISSIONS=true!");
		}
	}
#endif
}
