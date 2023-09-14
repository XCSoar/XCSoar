// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "UniqueHandle.hxx"
#include "Compat.hxx"
#include "util/ReturnValue.hxx"

#include <cassert>
#include <exception>
#include <utility>

namespace Co {

namespace detail {

template<typename R>
class promise_result_manager {
	ReturnValue<R> value;

public:
	template<typename U>
	void return_value(U &&_value) noexcept {
		value.Set(std::forward<U>(_value));
	}

	[[nodiscard]]
	decltype(auto) GetReturnValue() noexcept {
		return std::move(value).Get();
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
	[[nodiscard]]
	auto initial_suspend() noexcept {
		if constexpr (lazy)
			return std::suspend_always{};
		else
			return std::suspend_never{};
	}

	struct final_awaitable {
		[[nodiscard]]
		bool await_ready() const noexcept {
			return false;
		}

		template<typename PROMISE>
		[[nodiscard]]
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

	[[nodiscard]]
	auto final_suspend() noexcept {
		return final_awaitable{};
	}

	[[nodiscard]]
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

		[[nodiscard]]
		bool await_ready() const noexcept {
			return coroutine.done();
		}

		[[nodiscard]]
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

	[[nodiscard]]
	explicit EagerTask(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
	}

public:
	[[nodiscard]]
	EagerTask() = default;

	bool IsDefined() const noexcept {
		return coroutine;
	}

	[[nodiscard]]
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

	[[nodiscard]]
	explicit Task(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
	}

public:
	[[nodiscard]]
	Task() = default;

	bool IsDefined() const noexcept {
		return coroutine;
	}

	[[nodiscard]]
	typename promise_type::Awaitable operator co_await() const noexcept {
		return {coroutine.get()};
	}
};

} // namespace Co
