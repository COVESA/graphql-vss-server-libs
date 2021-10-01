#pragma once

#include <jwt-cpp/jwt.h>
#include <picojson/picojson.h>
#include <filesystem>

#include <list>
#include <mutex>
#include <string_view>

#include "authorizer.hpp"

#include "graphql_vss_server_libs-protocol_export.h"

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT JwtAuthorizer : public Authorizer
{
public:
	using Verifier = jwt::verifier<jwt::default_clock, jwt::picojson_traits>;
	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT
	JwtAuthorizer(Verifier&& jwtVerifier,
		std::unordered_map<std::string_view, ClientPermissions::Key>&& knownPermissions);

	JwtAuthorizer(JwtAuthorizer const&) = delete;
	JwtAuthorizer(JwtAuthorizer&&) = delete;

	// shared_ptr allows caching authorized tokens (same permissions => same ClientPermissions
	// pointer)
	const std::shared_ptr<const ClientPermissions> authorize(std::string&& token) override;
	const std::shared_ptr<const ClientPermissions> authorize(const std::string& token) override;

	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT static Verifier
	createDefaultVerifier(const std::string& pubKey);

	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT static Verifier
	createDefaultVerifier(const std::filesystem::path& path);

	// loads public key relative to executable location (dir) + PUB_KEY_PATH
	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT static Verifier createDefaultVerifier();

private:
	const Verifier m_jwtVerifier;
	const std::unordered_map<std::string_view, ClientPermissions::Key> m_knownPermissions;

	const std::shared_ptr<const ClientPermissions> m_emptyClientPermissions;
	// TODO: LRU cache with string PERMISSIONS_CLAIM => shared_ptr<ClientPermissions>

	static constexpr std::string_view PUB_KEY_PATH = "keys/jwtRS256.key.pub";
	static constexpr std::string_view PERMISSIONS_CLAIM = "permissions";
};
