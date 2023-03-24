// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "InvokeTask.hxx"
#include "event/InjectEvent.hxx"

#include <atomic>

namespace Co {

/**
 * Inject a coroutine into an #EventLoop.  The methods Start() and
 * Cancel() are thread-safe.
 */
class InjectTask {
	using Callback = InvokeTask::Callback;

	InjectEvent inject_event;

	InvokeTask task;
	Callback callback;

	std::atomic_bool alive{false};

public:
	explicit InjectTask(EventLoop &event_loop) noexcept;

	~InjectTask() noexcept {
		Cancel();
	}

	auto &GetEventLoop() const noexcept {
		return inject_event.GetEventLoop();
	}

	operator bool() const noexcept {
		return alive;
	}

	void Start(InvokeTask _task, Callback _callback) noexcept;

	/**
	 * Cancel coroutine execution.  After returning, the coroutine
	 * is guaranteed to be destroyed and its code (including the
	 * completion callback) is not running anymore.  This method
	 * is thread-safe.
	 */
	void Cancel() noexcept;

private:
	void OnInject() noexcept;
	void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace Co
