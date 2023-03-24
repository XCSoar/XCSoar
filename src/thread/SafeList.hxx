// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Mutex.hxx"

#include <forward_list>

/**
 * A thread-safe std::list which can be iterated while items are added
 * or removed.
 */
template<typename T>
class ThreadSafeList {
	/**
	 * This mutex protects both internal lists.  It is recursive
	 * to allow calling Add()/Remove() from within the ForEach()
	 * callback.
	 */
	RecursiveMutex mutex;

	/**
	 * A list of items whose callback by ForEach() is still
	 * pending.
	 */
	std::forward_list<T> pending;

	/**
	 * A list of items whose callback by ForEach() was already
	 * done.
	 */
	std::forward_list<T> done;

public:
	template<typename U>
	void Add(U &&value) noexcept {
		const std::lock_guard lock{mutex};
		pending.emplace_front(std::forward<U>(value));
	}

	void Remove(const auto &value) noexcept {
		const std::lock_guard lock{mutex};
		if (!pending.remove(value))
			done.remove(value);
	}

	void ForEach(auto &&f) noexcept {
		const std::lock_guard lock{mutex};

		/* move all "done" items back to "pending" */
		pending.splice_after(pending.before_begin(), done);

		/* now handle all "pending" items, moving each to
		   "done", until "pending" is empty; we're not using
		   iterators here because they may break when the
		   callback invokes Add() or Remove() */
		while (!pending.empty()) {
			done.splice_after(done.before_begin(), pending,
					  pending.before_begin());
			f(done.front());
		}
	}
};
