// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ReceiveTask.hpp"
#include "Task/XCTrackTaskDecoder.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/TaskBehaviour.hpp"
#include "thread/Mutex.hxx"
#include "util/StringCompare.hxx"

#include <boost/json.hpp>

#include <stdexcept>

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

  /* if XCSoar is already running, ask the UI thread to open the task
     manager; otherwise the task stays pending until it does */
  PostReceivedTask();
}

void
ReceiveTaskQRCode(std::string_view text)
{
  using std::string_view_literals::operator""sv;

  /* XCTrack writes the scheme in upper case ("XCTSK:"), whereas an
     Android intent URI arrives lower-cased - accept either */
  static constexpr auto xctrack_prefix = "XCTSK:"sv;
  if (!StringStartsWithIgnoreCase(text, xctrack_prefix))
    throw std::invalid_argument{"Not a task QR code"};

  ReceiveXCTrackTask(text.substr(xctrack_prefix.size()));
}
