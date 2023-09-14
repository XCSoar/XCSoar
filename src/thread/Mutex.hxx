// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <mutex>

#ifdef _WIN32

#include "CriticalSection.hxx"
using Mutex = CriticalSection;
using RecursiveMutex = CriticalSection;

#else

using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;

#endif

/**
 * Within the scope of an instance, this class will keep a #Mutex
 * unlocked.
 */
class ScopeUnlock {
	Mutex &mutex;

public:
	explicit ScopeUnlock(Mutex &_mutex) noexcept:mutex(_mutex) {
		mutex.unlock();
	}

	~ScopeUnlock() noexcept {
		mutex.lock();
	}

	ScopeUnlock(const ScopeUnlock &other) = delete;
	ScopeUnlock &operator=(const ScopeUnlock &other) = delete;
};
