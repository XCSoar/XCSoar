// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ReceiveTask.hpp"
#include "Task/XCTrackTaskDecoder.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/TaskBehaviour.hpp"
#include "thread/Mutex.hxx"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"

#include <boost/json.hpp>

static Mutex received_task_mutex;
static std::unique_ptr<OrderedTask> received_task;

bool
HasReceivedTask() noexcept
{
  const std::lock_guard lock{received_task_mutex};
  return !!received_task;
}

std::unique_ptr<OrderedTask>
GetReceivedTask() noexcept
{
  const std::lock_guard lock{received_task_mutex};
  return std::move(received_task);
}

void
ReceiveXCTrackTask(std::string_view data)
{
  // TODO use the configured TaskDefaults instance
  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  {
    auto task = DecodeXCTrackTask(boost::json::parse(data), task_behaviour);
    task->UpdateGeometry();
    task->SetName("XCTrack");
    const std::lock_guard lock{received_task_mutex};
    received_task = std::move(task);
  }

  /* if XCSoar is already running, post a TASK_RECEIVED event so
     MainWindow::OnTaskReceived() opens the task manager */
  if (UI::event_queue != nullptr)
    UI::event_queue->Inject(UI::Event::TASK_RECEIVED);
}
