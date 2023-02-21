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
		Callback callback;

		std::exception_ptr error;

		auto initial_suspend() noexcept {
			assert(!error);

			return std::suspend_always{};
		}

		struct final_awaitable {
			bool await_ready() const noexcept {
				return false;
			}

			template<typename PROMISE>
			void await_suspend(std::coroutine_handle<PROMISE> coro) noexcept {
				assert(coro);
				assert(coro.done());

				auto &p = coro.promise();
				assert(p.callback);
				p.callback(std::move(p.error));
			}

			void await_resume() const noexcept {
			}
		};

		auto final_suspend() noexcept {
			return final_awaitable{};
		}

		void return_void() noexcept {
			assert(!error);
		}

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

	explicit InvokeTask(std::coroutine_handle<promise_type> _coroutine) noexcept
		:coroutine(_coroutine)
	{
	}

public:
	InvokeTask() noexcept {
	}

	operator bool() const noexcept {
		return coroutine;
	}

	bool done() const noexcept {
		assert(coroutine);

		return coroutine->done();
	}

	void Start(Callback callback) noexcept {
		assert(callback);
		assert(coroutine);
		assert(!coroutine->done());
		assert(!coroutine->promise().error);

		coroutine->promise().callback = callback;
		coroutine->resume();
	}
};

} // namespace Co
