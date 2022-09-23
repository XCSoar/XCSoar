/*
 * Copyright 2022 Max Kellermann <max.kellermann@gmail.com>
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
