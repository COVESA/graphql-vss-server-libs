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

#include "commonapi-singletons.hpp"

#ifndef GRAPHQL_SOMEIP_NAME
#define GRAPHQL_SOMEIP_NAME "graphql"
#endif

std::shared_ptr<CommonAPI::Runtime> commonAPISingletonProxyRuntime = CommonAPI::Runtime::get();
std::string commonAPISingletonProxyConnectionId(GRAPHQL_SOMEIP_NAME);
std::string commonAPISingletonProxyDomain("local");
