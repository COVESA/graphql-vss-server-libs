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
