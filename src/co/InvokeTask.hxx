// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "UniqueHandle.hxx"
#include "Compat.hxx"
#include "util/BindMethod.hxx"

#include <cassert>
#include <exception>
#include <utility>

namespace Co {

namespace detail {

template<typename Task>
class InvokePromise {
	friend Task;

	using Callback = BoundMethod<void(std::exception_ptr error) noexcept>;

	Task *task;

	Callback callback;

	std::exception_ptr error;

public:
	[[nodiscard]]
	auto initial_suspend() noexcept {
		assert(!error);

		return std::suspend_always{};
	}

private:
	struct final_awaitable {
		[[nodiscard]]
		bool await_ready() const noexcept {
			return false;
		}

		template<typename PROMISE>
		bool await_suspend(std::coroutine_handle<PROMISE> coro) noexcept {
			assert(coro);
			assert(coro.done());

			auto &p = coro.promise();
			assert(p.task);
			assert(p.task->coroutine);
			assert(p.callback);

			/* release the coroutine_handle; it will be
			   destroyed by our caller */
			(void)p.task->coroutine.release();

			p.callback(std::move(p.error));

			/* this resumes the original coroutine which
			   will then destroy the coroutine_handle */
			return false;
		}

		void await_resume() const noexcept {
		}
	};

public:
	[[nodiscard]]
	auto final_suspend() noexcept {
		return final_awaitable{};
	}

	void return_void() noexcept {
		assert(!error);
	}

	[[nodiscard]]
	Task get_return_object() noexcept {
		assert(!error);

		return Task{std::coroutine_handle<InvokePromise>::from_promise(*this)};
	}

	void unhandled_exception() noexcept {
		assert(!error);
		error = std::current_exception();
	}
};

} // namespace detail

/**
 * A helper task which invokes a coroutine from synchronous code.
 */
class InvokeTask {
public:
	using promise_type = detail::InvokePromise<InvokeTask>;
	friend promise_type;

	using Callback = promise_type::Callback;

private:
	UniqueHandle<promise_type> coroutine;

	[[nodiscard]]
	explicit InvokeTask(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
	}

public:
	[[nodiscard]]
	InvokeTask() noexcept {
	}

	operator bool() const noexcept {
		return coroutine;
	}

	void Start(Callback callback) noexcept {
		assert(callback);
		assert(coroutine);
		assert(!coroutine->done());
		assert(!coroutine->promise().error);

		coroutine->promise().task = this;
		coroutine->promise().callback = callback;
		coroutine->resume();
	}
};

} // namespace Co
