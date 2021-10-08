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
