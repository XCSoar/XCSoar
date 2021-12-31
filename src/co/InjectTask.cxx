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

#include "InjectTask.hxx"
#include "event/Call.hxx"

#include <cassert>

namespace Co {

InjectTask::InjectTask(EventLoop &event_loop) noexcept
	:inject_event(event_loop, BIND_THIS_METHOD(OnInject)) {}

void
InjectTask::Start(InvokeTask _task, Callback _callback) noexcept
{
	assert(!alive);
	assert(_task);
	assert(_callback);

	task = std::move(_task);
	callback = _callback;
	alive = true;

	inject_event.Schedule();
}

void
InjectTask::Cancel() noexcept
{
	if (!alive) {
		assert(!task);
		return;
	}

	inject_event.Cancel();

	BlockingCall(GetEventLoop(), [this](){
		/* this actually cancels and disposes the coroutine
		   which is already running */
		task = {};
	});

	alive = false;
}

void
InjectTask::OnInject() noexcept
{
	assert(alive);
	assert(task);

	task.Start(BIND_THIS_METHOD(OnCompletion));
}

void
InjectTask::OnCompletion(std::exception_ptr error) noexcept
{
	assert(alive);
	assert(task);

	task = {};
	alive = false;

	callback(std::move(error));
}

} // namespace Co
