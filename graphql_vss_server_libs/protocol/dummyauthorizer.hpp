#pragma once

#include <unordered_map>

#include "authorizer.hpp"

#include "graphql_vss_server_libs-protocol_export.h"

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT DummyAuthorizer final : public Authorizer
{
public:
	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT
	DummyAuthorizer(
		std::unordered_map<std::string_view, ClientPermissions::Key>&& knownPermissions);

	DummyAuthorizer(DummyAuthorizer const&) = delete;
	DummyAuthorizer(DummyAuthorizer&&) = delete;

	const std::shared_ptr<const ClientPermissions> authorize(std::string&& token) override;
	const std::shared_ptr<const ClientPermissions> authorize(const std::string& token) override;

private:
	std::shared_ptr<ClientPermissions> m_permissions;
};
