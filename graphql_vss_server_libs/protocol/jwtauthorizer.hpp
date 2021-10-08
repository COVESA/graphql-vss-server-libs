// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi),
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

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
