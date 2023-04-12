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

/**
 * A helper task which invokes a coroutine from synchronous code.
 */
class InvokeTask {
public:
	using Callback = BoundMethod<void(std::exception_ptr error) noexcept>;

	struct promise_type {
		InvokeTask *task;

		Callback callback;

		std::exception_ptr error;

		[[nodiscard]]
		auto initial_suspend() noexcept {
			assert(!error);

			return std::suspend_always{};
		}

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

				(void)p.task->coroutine.release();
				p.callback(std::move(p.error));

				return false;
			}

			void await_resume() const noexcept {
			}
		};

		[[nodiscard]]
		auto final_suspend() noexcept {
			return final_awaitable{};
		}

		void return_void() noexcept {
			assert(!error);
		}

		[[nodiscard]]
		InvokeTask get_return_object() noexcept {
			assert(!error);

			return InvokeTask(std::coroutine_handle<promise_type>::from_promise(*this));
		}

		void unhandled_exception() noexcept {
			assert(!error);
			error = std::current_exception();
		}
	};

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
