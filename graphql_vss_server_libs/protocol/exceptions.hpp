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

#include <exception>
#include <string_view>
#include <string>

#include "graphql_vss_server_libs-protocol_export.h"

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidPayload : public std::exception
{
    const std::string m_message;

public:
    GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidPayload(const std::string& exceptionMessage);
    const char* what() const noexcept override;

    constexpr static std::string_view messagePrefix = "Invalid payload: ";
};

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidToken : public std::exception
{
    const std::string m_message;

public:
    GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidToken(const std::string& exceptionMessage);
    const char* what() const noexcept override;

    constexpr static std::string_view messagePrefix = "Token error: ";
};

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT ContextException : public std::exception
{
public:
    const char* what() const noexcept override;
};
