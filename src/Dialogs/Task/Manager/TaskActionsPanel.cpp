// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskActionsPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskListPanel.hpp"
#include "Internal.hpp"
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/Message.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Device/Declaration.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Task/QRScanner.hpp"
#ifdef HAVE_ZXING
#include "Dialogs/Task/TaskQRCodeDialog.hpp"
#endif
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"

TaskActionsPanel::TaskActionsPanel(TaskManagerDialog &_dialog,
                                   TaskMiscPanel &_parent,
                                   std::unique_ptr<OrderedTask> &_active_task,
                                   bool *_task_modified) noexcept
  :RowFormWidget(_dialog.GetLook()),
   dialog(_dialog), parent(_parent),
   active_task(_active_task), task_modified(_task_modified) {}

void
TaskActionsPanel::SaveTask()
{
  AbstractTaskFactory &factory = active_task->GetFactory();
  factory.UpdateStatsGeometry();
  if (factory.CheckAddFinish())
    factory.UpdateGeometry();

  const auto errors = active_task->CheckTask();
  if (!IsError(errors)) {
    if (!OrderedTaskSave(*active_task))
      return;

    *task_modified = true;
    dialog.UpdateCaption();
    DirtyTaskListPanel();
  } else {
    ShowMessageBox(getTaskValidationErrors(errors), _("Task not saved"),
        MB_ICONEXCLAMATION);
  }
}

inline void
TaskActionsPanel::OnBrowseClicked()
{
  parent.SetCurrent(parent.PAGE_LIST);
}

inline void
TaskActionsPanel::OnNewTaskClicked()
{
  if ((active_task->TaskSize() < 2) ||
      (ShowMessageBox(_("Create new task?"), _("Task New"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    active_task->Clear();
    active_task->SetFactory(CommonInterface::GetComputerSettings().task.task_type_default);
    *task_modified = true;
    dialog.SwitchToPropertiesPanel();
  }
}

inline void
TaskActionsPanel::OnDeclareClicked()
{
  const auto errors = active_task->CheckTask();
  if (IsError(errors)) {
    ShowMessageBox(getTaskValidationErrors(errors), _("Declare task"),
                MB_ICONEXCLAMATION);
    return;
  }

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  Declaration decl(settings.logger, settings.plane, active_task.get());
  ExternalLogger::Declare(decl, data_components->waypoints->GetHome().get());
}

#ifdef HAVE_ZXING
inline void
TaskActionsPanel::OnShowQRCodeClicked() noexcept
{
  ShowTaskQRCode(*active_task);
}
#endif

/**
 * Open the camera, leaving this dialog open behind it.  The scan
 * reports back asynchronously, so closing here would throw away the
 * pilot's task if they then cancelled the scan.  A task that does
 * arrive is handed to this still-open dialog by
 * MainWindow::OnTaskReceived() via TaskManagerReceiveTask().
 */
inline void
TaskActionsPanel::OnScanQRCodeClicked() noexcept
{
  ScanTaskQRCode();
}

void
TaskActionsPanel::ReClick() noexcept
{
  dialog.TaskViewClicked();
}

void
TaskActionsPanel::Prepare([[maybe_unused]] ContainerWindow &_parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton(_("New Task"), [this](){ OnNewTaskClicked(); });
  AddButton(_("Declare"), [this](){ OnDeclareClicked(); });
  AddButton(_("Browse"), [this](){ OnBrowseClicked(); });
  AddButton(_("Save"), [this](){ SaveTask(); });

#ifdef HAVE_ZXING
  AddButton(_("Show QR Code"), [this](){ OnShowQRCodeClicked(); });
#endif
#ifdef HAVE_HTTP
  AddButton(_("SoaringScoring"), [this](){
    parent.SetCurrent(parent.PAGE_SOARING_SCORING);
  });
#endif

  if (HaveQRScanner())
    AddButton(_("Scan QR Code"), [this](){ OnScanQRCodeClicked(); });

  if (is_simulator())
    /* cannot communicate with real devices in simulator mode */
    SetRowEnabled(DECLARE, false);
}
