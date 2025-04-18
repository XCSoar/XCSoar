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

template<typename Task, bool lazy>
class InvokePromise {
	friend Task;

	using Callback = BoundMethod<void(std::exception_ptr error) noexcept>;

	Task *task;

	Callback callback;

	std::exception_ptr error;

public:
	InvokePromise() noexcept requires(lazy) = default;

	InvokePromise() noexcept requires(!lazy)
		:callback(nullptr) {}

	InvokePromise(const InvokePromise &) = delete;
	InvokePromise &operator=(const InvokePromise &) = delete;

	[[nodiscard]]
	auto initial_suspend() noexcept {
		assert(!error);

		if constexpr (lazy)
			return std::suspend_always{};
		else
			return std::suspend_never{};
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

			if constexpr (!lazy) {
				if (!p.callback)
					return true;
			}


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

	void unhandled_exception() noexcept(lazy) {
		assert(!error);

		if constexpr (!lazy) {
			if (!callback) {
				/* the coroutine_handle will be
				   destroyed by the compiler after
				   rethrowing the exception */
				(void)task->coroutine.release();
				throw;
			}
		}

		error = std::current_exception();
	}
};

} // namespace detail

/**
 * A helper task which invokes a coroutine from synchronous code.
 */
class InvokeTask {
public:
	using promise_type = detail::InvokePromise<InvokeTask, true>;
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

/**
 * Like #InvokeTask, but the coroutine is not suspended initially.
 */
class EagerInvokeTask {
public:
	using promise_type = detail::InvokePromise<EagerInvokeTask, false>;
	friend promise_type;

	using Callback = promise_type::Callback;

private:
	UniqueHandle<promise_type> coroutine;

	[[nodiscard]]
	explicit EagerInvokeTask(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
		/* initialize promise.task early because its exception
		   handler needs to release the coroutine handle */
		coroutine->promise().task = this;
	}

public:
	[[nodiscard]]
	EagerInvokeTask() noexcept = default;

	EagerInvokeTask(EagerInvokeTask &&src) noexcept
		:coroutine(std::move(src.coroutine))
	{
		if (coroutine) {
			assert(coroutine->promise().task == &src);
			coroutine->promise().task = this;
		}
	}

	EagerInvokeTask &operator=(EagerInvokeTask &&src) noexcept {
		coroutine = std::move(src.coroutine);
		if (coroutine) {
			assert(coroutine->promise().task == &src);
			coroutine->promise().task = this;
		}

		return *this;
	}

	operator bool() const noexcept {
		return coroutine;
	}

	void Start(Callback callback) noexcept {
		assert(callback);
		assert(coroutine);
		assert(coroutine->promise().task == this);
		assert(!coroutine->promise().error);

		if (coroutine->done()) {
			coroutine = {};
			callback({});
		} else {
			coroutine->promise().callback = callback;

			/* not calling "resume()" here because the
			   coroutine was already resumed when it was
			   constructed; since it is not "done" yet, it
			   must be suspended currently */
		}
	}
};

} // namespace Co
