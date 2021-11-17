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

#include "exceptions.hpp"

InvalidPayload::InvalidPayload(const std::string& message)
    : m_message(std::string { messagePrefix } + message)
{
}

const char* InvalidPayload::what() const noexcept
{
    return m_message.c_str();
}

InvalidToken::InvalidToken(const std::string& message)
    : m_message(std::string { messagePrefix } + message)
{
}

const char* InvalidToken::what() const noexcept
{
    return m_message.c_str();
}

const char* ContextException::what() const noexcept
{
    return "Client not authenticated";
}
