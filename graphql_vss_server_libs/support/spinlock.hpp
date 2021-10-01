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
