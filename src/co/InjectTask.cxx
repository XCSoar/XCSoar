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

	++generation;

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

  const unsigned cancel_generation = generation.load();

  BlockingCall(GetEventLoop(), [this, cancel_generation](){
    /* dispose the coroutine only if no new Start() happened
       while this cancel was waiting for the event loop */
    if (generation.load() == cancel_generation)
      task = {};
  });

  if (generation.load() == cancel_generation)
    alive = false;
}

void
InjectTask::OnInject() noexcept
{
	if (!alive || !task)
		return;

	task.Start(BIND_THIS_METHOD(OnCompletion));
}

void
InjectTask::OnCompletion(std::exception_ptr error) noexcept
{
	if (!alive)
		return;

	alive = false;

	callback(std::move(error));
}

} // namespace Co
