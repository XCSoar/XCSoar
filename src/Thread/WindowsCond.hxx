/*
 * Copyright (C) 2009-2013 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THREAD_WINDOWS_COND_HXX
#define THREAD_WINDOWS_COND_HXX

#include "CriticalSection.hxx"
#include "Mutex.hpp"

/**
 * Wrapper for a CONDITION_VARIABLE, backend for the Cond class.
 */
class WindowsCond {
	CONDITION_VARIABLE cond;

public:
	WindowsCond() {
		InitializeConditionVariable(&cond);
	}

	WindowsCond(const WindowsCond &other) = delete;
	WindowsCond &operator=(const WindowsCond &other) = delete;

	void signal() {
		WakeConditionVariable(&cond);
	}

	void broadcast() {
		WakeAllConditionVariable(&cond);
	}

	bool timed_wait(CriticalSection &mutex, DWORD timeout_ms) {
		return SleepConditionVariableCS(&cond, &mutex.critical_section,
						timeout_ms);
	}

	bool timed_wait(Mutex &mutex, unsigned timeout_ms) {
		TemporaryUnlock unlock(mutex);
		return timed_wait(mutex.mutex, timeout_ms);
	}

	void wait(CriticalSection &mutex) {
		timed_wait(mutex, INFINITE);
	}

	void wait(Mutex &mutex) {
		TemporaryUnlock unlock(mutex);
		wait(mutex.mutex);
	}
};

#endif
