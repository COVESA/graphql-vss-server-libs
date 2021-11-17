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

#include <thread>
#include <atomic>

#include "debug.hpp"
#include "graphql_vss_server_libs-support_export.h"

// SpinLock is compliant with std::lock_guard (BasicLockable)
struct SpinLock
{
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;

#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG_LOCKS
#define THE_DEBUG_TARGET m_debug_target
    // Target is only used in debug, it's not used in any other way
    const void* THE_DEBUG_TARGET;
#else
#define THE_DEBUG_TARGET nullptr
#endif

    SpinLock(const void* debug_target)
#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG_LOCKS
        : THE_DEBUG_TARGET(debug_target)
#endif
    {
        (void)debug_target;
    };

    SpinLock(SpinLock&& other) = delete;

    inline void lock()
    {
        debug_will_lock(m_flag, THE_DEBUG_TARGET);
        // use a spin lock as contention is very unlikely, thus cheaper than a mutex
        while (m_flag.test_and_set(std::memory_order_acquire))
            std::this_thread::yield();
        debug_did_lock(m_flag, THE_DEBUG_TARGET);
    }

    inline void unlock()
    {
        debug_will_unlock(m_flag, THE_DEBUG_TARGET);
        m_flag.clear(std::memory_order_release);
        debug_did_unlock(m_flag, THE_DEBUG_TARGET);
    }

#undef THE_DEBUG_TARGET
};
