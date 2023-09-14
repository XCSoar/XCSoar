// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <synchapi.h>

/**
 * Low-level wrapper for a SRWLOCK.
 */
class WindowsSharedMutex {
	SRWLOCK srwlock = SRWLOCK_INIT;

public:
	constexpr WindowsSharedMutex() = default;

	WindowsSharedMutex(const WindowsSharedMutex &other) = delete;
	WindowsSharedMutex &operator=(const WindowsSharedMutex &other) = delete;

	void lock() {
		AcquireSRWLockExclusive(&srwlock);
	}

	bool try_lock() {
		return TryAcquireSRWLockExclusive(&srwlock);
	}

	void unlock() {
		ReleaseSRWLockExclusive(&srwlock);
	}

	void lock_shared() {
		AcquireSRWLockShared(&srwlock);
	}

	bool try_lock_shared() {
		return TryAcquireSRWLockShared(&srwlock);
	}

	void unlock_shared() {
		ReleaseSRWLockShared(&srwlock);
	}
};
