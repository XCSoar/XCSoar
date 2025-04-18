// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cassert>
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
		assert(cancellable != nullptr);

		std::exchange(cancellable, nullptr)->Cancel();
	}
};
