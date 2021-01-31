/*
 * Copyright 2016-2019 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef CANCELLABLE_HXX
#define CANCELLABLE_HXX

#include <utility>

/**
 * An asynchronous operation that can be cancelled.  Upon
 * cancellation, the operation's handler must not be invoked.
 */
class Cancellable {
public:
	virtual void Cancel() noexcept = 0;
};

class CancellablePointer {
	Cancellable *cancellable = nullptr;

public:
	constexpr CancellablePointer() = default;

	constexpr CancellablePointer(std::nullptr_t n) noexcept
		:cancellable(n) {}

	CancellablePointer(CancellablePointer &&src) noexcept
		:cancellable(std::exchange(src.cancellable, nullptr)) {}

	CancellablePointer &operator=(CancellablePointer &&src) noexcept {
		using std::swap;
		swap(cancellable, src.cancellable);
		return *this;
	}

	CancellablePointer &operator=(std::nullptr_t n) noexcept {
		cancellable = n;
		return *this;
	}

	CancellablePointer &operator=(Cancellable &_cancellable) noexcept {
		cancellable = &_cancellable;
		return *this;
	}

	constexpr operator bool() const noexcept {
		return cancellable != nullptr;
	}

	void Cancel() noexcept {
		cancellable->Cancel();
	}

	void CancelAndClear() noexcept {
		auto *c = cancellable;
		cancellable = nullptr;
		c->Cancel();
	}
};

#endif
