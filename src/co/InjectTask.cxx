// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

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

	alive = false;

	callback(std::move(error));
}

} // namespace Co
