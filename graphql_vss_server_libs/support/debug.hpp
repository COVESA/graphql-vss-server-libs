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

#include <iostream>

#include "graphql_vss_server_libs-support_export.h"

#if !GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG_COLORS
#define COLOR_RESET ""
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN ""
#define COLOR_BG_RED ""
#define COLOR_BG_GREEN ""
#define COLOR_BG_YELLOW ""
#define COLOR_BG_BLUE ""
#define COLOR_BG_MAGENTA ""
#define COLOR_BG_CYAN ""
#else
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_BG_RED "\033[1;41m"
#define COLOR_BG_GREEN "\033[1;42m"
#define COLOR_BG_YELLOW "\033[1;43m"
#define COLOR_BG_BLUE "\033[1;44m"
#define COLOR_BG_MAGENTA "\033[1;45m"
#define COLOR_BG_CYAN "\033[1;46m"
#endif

// DEBUG will define dbg() to print to stderr lines prefixed with:
//    DBG:[T:1234] Message
// where 1234 is the thread id. It's locked and safe against other
// threads using dbg() at the same time.
#if !GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
// NOTE: keep 'if (0)' so 'stmt' is validated even if unused, also it will
// avoid "uninitialized variables" if they were just used to debug. This
// should impose no runtime penalty as optimizations will remove such blocks
#define dbg(stmt)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        if (0)                                                                                     \
        {                                                                                          \
            std::cerr << stmt;                                                                     \
        }                                                                                          \
    } while (0)
#else
#include <thread>
#include <mutex>
#include <sstream>
extern GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::mutex _dbg_lock;
#define dbg(stmt)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        std::lock_guard<std::mutex> guard(_dbg_lock);                                              \
        std::ostringstream dbg_message;                                                            \
        dbg_message << "DBG:[T:" << std::this_thread::get_id() << "] " << stmt << COLOR_RESET      \
                    << std::endl;                                                                  \
        std::cerr << dbg_message.str();                                                            \
    } while (0)
#endif

// DEBUG_LOCKS will print out the locks and unlocks as well as introduce
// a DEBUG_LOCK_TIMEOUT (100ms) sleep to give more time for other threads to
// compete for it and exercise locks
#if !GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG_LOCKS
#define debug_will_lock(lock, what)                                                                \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#define debug_did_lock(lock, what)                                                                 \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#define debug_will_unlock(lock, what)                                                              \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#define debug_did_unlock(lock, what)                                                               \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#else
#include <thread>
#include <chrono>
#define DEBUG_LOCK_TIMEOUT std::chrono::milliseconds(100)
#define debug_will_lock(lock, what)                                                                \
    do                                                                                             \
    {                                                                                              \
        dbg(COLOR_BLUE ">>> will lock " << (what) << " using " << (&lock) << "...");               \
        std::this_thread::sleep_for(DEBUG_LOCK_TIMEOUT);                                           \
    } while (0)
#define debug_did_lock(lock, what)                                                                 \
    do                                                                                             \
    {                                                                                              \
        dbg(COLOR_YELLOW ">>> did lock " << (what) << " using " << (&lock) << "!");                \
        std::this_thread::sleep_for(DEBUG_LOCK_TIMEOUT);                                           \
    } while (0)
#define debug_will_unlock(lock, what)                                                              \
    do                                                                                             \
    {                                                                                              \
        dbg(COLOR_YELLOW "<<< will unlock " << (what) << " using " << (&lock) << "...");           \
        std::this_thread::sleep_for(DEBUG_LOCK_TIMEOUT);                                           \
    } while (0)
#define debug_did_unlock(lock, what)                                                               \
    do                                                                                             \
    {                                                                                              \
        dbg(COLOR_BLUE "<<< did unlock " << (what) << " using " << (&lock) << "!");                \
        std::this_thread::sleep_for(DEBUG_LOCK_TIMEOUT);                                           \
    } while (0)
#endif
