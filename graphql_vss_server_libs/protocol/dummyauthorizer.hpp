// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

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
