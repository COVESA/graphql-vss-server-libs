#include <graphql_vss_server_libs/support/debug.hpp>

#include "dummyauthorizer.hpp"

DummyAuthorizer::DummyAuthorizer(
	std::unordered_map<std::string_view, ClientPermissions::Key>&& knownPermissions)
	: Authorizer(),
	m_permissions(std::make_shared<ClientPermissions>())
{
	dbg(COLOR_BG_BLUE << "DummyAuthorizer created. Will allow everything");
	for (auto& it : knownPermissions)
	{
		m_permissions->insert(it.second);
	}
}

// Ignores the token
const std::shared_ptr<const ClientPermissions> DummyAuthorizer::authorize(std::string&& token)
{
	(void)token;
	return m_permissions;
}

const std::shared_ptr<const ClientPermissions> DummyAuthorizer::authorize(const std::string& token)
{
	(void)token;
	return m_permissions;
}
