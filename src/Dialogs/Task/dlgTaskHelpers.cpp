// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgTaskHelpers.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Language/Language.hpp"
#include "Task/SaveFile.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"

#include <cstring>

bool
OrderedTaskSave(OrderedTask &task)
{
  char fname[69] = "";
  if (!TextEntryDialog(fname, 64, _("Enter a task name")))
    return false;

  const auto tasks_path = MakeLocalPath("tasks");

  strcat(fname, ".tsk");
  task.SetName(fname);
  SaveTask(AllocatedPath::Build(tasks_path, fname), task);
  return true;
}
