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
