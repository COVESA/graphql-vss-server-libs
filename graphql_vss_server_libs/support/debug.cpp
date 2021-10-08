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

#include "debug.hpp"

#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
// NOTE: this must be before the first dbg() user, so keep this file the first to be compiled!
// otherwise you may get the lock to be cleaned up BEFORE the users, resulting in:
//    terminating with uncaught exception of type std::__1::system_error: mutex lock failed: Invalid
//    argument
std::mutex _dbg_lock;
#endif
