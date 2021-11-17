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

#include <memory>
#include <set>

#include "graphql_vss_server_libs-support_export.h"

struct GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT PermissionException : public std::exception
{
    const char* what() const noexcept override;
};

class ClientPermissions
{
public:
    using Key = uint16_t;

    GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void insert(const Key& permission);

    ClientPermissions() = default;
    ClientPermissions(ClientPermissions const&) = delete;
    ClientPermissions(ClientPermissions&&) = delete;

    inline bool contains(const Key& permission) const noexcept
    {
        return m_lookup.find(permission) != m_lookup.cend();
    }

    template <typename... T>
    inline void validate(const Key& permission, const T&... rest) const
    {
        if (!contains(permission))
        {
            throw PermissionException();
        }
        if constexpr (sizeof...(rest) > 0)
        {
            validate(rest...);
        }
    }

private:
    // How the Request stores the client permissions,
    // must have an efficient lookup!
    std::set<Key> m_lookup;
};
