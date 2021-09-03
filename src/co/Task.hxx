/*
 * Copyright 2020-2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
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

#include "UniqueHandle.hxx"
#include "Compat.hxx"

#include <cassert>
#include <exception>
#include <optional>
#include <utility>

namespace Co {

namespace detail {

template<typename R>
class promise_result_manager {
	std::optional<R> value;

public:
	template<typename U>
	void return_value(U &&_value) noexcept {
		assert(!value);

		value.emplace(std::forward<U>(_value));
	}

	decltype(auto) GetReturnValue() noexcept {
		/* this assertion can fail if control flows off the
		   end of a coroutine without co_return, which is
		   undefined behavior according to
		   https://timsong-cpp.github.io/cppwp/n4861/stmt.return.coroutine */
		assert(value);

		return std::move(*value);
	}
};

template<>
class promise_result_manager<void> {
public:
	void return_void() noexcept {}
	void GetReturnValue() noexcept {}
};

template<typename T, typename Task, bool lazy>
class promise final : public detail::promise_result_manager<T> {
	std::coroutine_handle<> continuation;

	std::exception_ptr error;

public:
	auto initial_suspend() noexcept {
		if constexpr (lazy)
			return std::suspend_always{};
		else
			return std::suspend_never{};
	}

	struct final_awaitable {
		bool await_ready() const noexcept {
			return false;
		}

		template<typename PROMISE>
		std::coroutine_handle<> await_suspend(std::coroutine_handle<PROMISE> coro) noexcept {
			const auto &promise = coro.promise();

			if (!lazy && !promise.continuation)
				/* if this is an "eager" task, the
				   continuation may not yet have been
				   set, because nobody has co_awaited
				   the task yet */
				return std::noop_coroutine();

			assert(promise.continuation);

			return promise.continuation;
		}

		void await_resume() noexcept {
		}
	};

	auto final_suspend() noexcept {
		return final_awaitable{};
	}

	auto get_return_object() noexcept {
		return Task(std::coroutine_handle<promise>::from_promise(*this));
	}

	void unhandled_exception() noexcept {
		error = std::current_exception();
	}

private:
	void SetContinuation(std::coroutine_handle<> _continuation) noexcept {
		assert(!continuation);
		assert(_continuation);

		continuation = _continuation;
	}

	decltype(auto) GetReturnValue() {
		if (error)
			std::rethrow_exception(std::move(error));

		return detail::promise_result_manager<T>::GetReturnValue();
	}

public:
	struct Awaitable final {
		const std::coroutine_handle<promise> coroutine;

		bool await_ready() const noexcept {
			return coroutine.done();
		}

		std::coroutine_handle<> await_suspend(std::coroutine_handle<> _continuation) noexcept {
			coroutine.promise().SetContinuation(_continuation);

			if constexpr (!lazy)
				/* if this is an "eager" task, then
				   the coroutine has already been
				   resumed once, and is awaiting
				   another task right now, so do
				   nothing now */
				return std::noop_coroutine();

			return coroutine;
		}

		T await_resume() {
			return coroutine.promise().GetReturnValue();
		}
	};
};

} // namespace Co::detail

/**
 * A coroutine task which is not suspended initially ("eager") and
 * returns a value (with support for exceptions).
 */
template<typename T>
class EagerTask {
public:
	using promise_type = detail::promise<T, EagerTask<T>, false>;
	friend promise_type;

private:
	UniqueHandle<promise_type> coroutine;

	explicit EagerTask(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
	}

public:
	EagerTask() = default;

	typename promise_type::Awaitable operator co_await() const noexcept {
		return {coroutine.get()};
	}
};

/**
 * A coroutine task which is suspended initially and returns a value
 * (with support for exceptions).
 */
template<typename T>
class Task {
public:
	using promise_type = detail::promise<T, Task<T>, true>;
	friend promise_type;

private:
	UniqueHandle<promise_type> coroutine;

	explicit Task(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
	}

public:
	Task() = default;

	typename promise_type::Awaitable operator co_await() const noexcept {
		return {coroutine.get()};
	}
};

} // namespace Co
