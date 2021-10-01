#pragma once

#include <string>

#include <graphql_vss_server_libs/support/permissions.hpp>

struct Authorizer
{
	// shared_ptr allows caching authorized tokens (same permissions => same ClientPermissions
	// pointer)
	virtual const std::shared_ptr<const ClientPermissions> authorize(std::string&& token) = 0;
	virtual const std::shared_ptr<const ClientPermissions> authorize(const std::string& token) = 0;
};
