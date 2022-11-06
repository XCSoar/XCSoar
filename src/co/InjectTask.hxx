/*
 * Copyright 2021 Max Kellermann <max.kellermann@gmail.com>
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
