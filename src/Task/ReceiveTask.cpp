// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ReceiveTask.hpp"

#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/TaskBehaviour.hpp"
#include "Task/CupTaskQR.hpp"
#include "Task/XCSoarTaskQR.hpp"
#include "Task/XCTrackTaskDecoder.hpp"
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

static void
StoreReceivedTask(std::unique_ptr<OrderedTask> task)
{
  task->UpdateGeometry();
  {
    const std::lock_guard lock{received_task_mutex};
    received_task = std::move(task);
  }

  /* if XCSoar is already running, ask the UI thread to open the task
     manager; otherwise the task stays pending until it does */
  PostReceivedTask();
}

void
ReceiveXCTrackTask(std::string_view data)
{
  // TODO use the configured TaskDefaults instance
  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  auto task = DecodeXCTrackTask(boost::json::parse(data), task_behaviour);
  task->SetName("XCTrack");
  StoreReceivedTask(std::move(task));
}

static void
ReceiveCupTask(std::string_view cup)
{
  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  StoreReceivedTask(DecodeCupTaskQRText(cup, task_behaviour));
}

static void
ReceiveXCSoarTask(std::string_view xml)
{
  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  StoreReceivedTask(DecodeXCSoarTaskQRText(xml, task_behaviour));
}

void
ReceiveTaskQRCode(std::string_view text)
{
  using std::string_view_literals::operator""sv;

  /* XCTrack writes the scheme in upper case ("XCTSK:"), whereas an
     Android intent URI arrives lower-cased - accept either */
  static constexpr auto xctrack_prefix = "XCTSK:"sv;
  if (StringStartsWithIgnoreCase(text, xctrack_prefix)) {
    ReceiveXCTrackTask(text.substr(xctrack_prefix.size()));
    return;
  }

  /* Same as XCTSK: the scheme in an Android intent is lower case
     ("xcs:"), while the QR code we draw uses "XCS:".  Only those four
     characters are compared that way; the task XML after them is
     case-sensitive. */
  if (StringStartsWithIgnoreCase(text, XCSOAR_TASK_QR_PREFIX)) {
    ReceiveXCSoarTask(text.substr(XCSOAR_TASK_QR_PREFIX.size()));
    return;
  }

  /* "CUP:" in the code we draw, "cup:" from an Android intent. */
  if (StringStartsWithIgnoreCase(text, CUP_TASK_QR_PREFIX)) {
    ReceiveCupTask(text.substr(CUP_TASK_QR_PREFIX.size()));
    return;
  }

  throw std::invalid_argument{"Not a task QR code"};
}
