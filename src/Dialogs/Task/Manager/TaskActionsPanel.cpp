/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TaskActionsPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskListPanel.hpp"
#include "Internal.hpp"
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/Message.hpp"
#include "Components.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Device/Declaration.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "net/http/DownloadManager.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "LogFile.hpp"
#include "../../WidgetDialog.hpp"
#include "../../ProgressDialog.hpp"
#include "net/http/Features.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "UIGlobals.hpp"

/**
 * This class tracks a download and updates a #ProgressDialog.
 */
class WeGlideDownloadProgress final : Net::DownloadListener {
  ProgressDialog &dialog;
  ThreadedOperationEnvironment env;
  const Path path_relative;

  UI::PeriodicTimer update_timer{[this]{ Net::DownloadManager::Enumerate(*this); }};

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  std::exception_ptr error;

  bool got_size = false, complete = false, success;

public:
  WeGlideDownloadProgress(ProgressDialog &_dialog,
                   const Path _path_relative)
    :dialog(_dialog), env(_dialog), path_relative(_path_relative) {
    update_timer.Schedule(std::chrono::seconds(1));
    Net::DownloadManager::AddListener(*this);
  }

  ~WeGlideDownloadProgress() {
    Net::DownloadManager::RemoveListener(*this);
  }

  void Rethrow() const {
    if (error)
      std::rethrow_exception(error);
  }

private:
  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path _path_relative,
                       int64_t size, int64_t position) noexcept override {
    if (!complete && path_relative == _path_relative) {
      if (!got_size && size >= 0) {
        got_size = true;
        env.SetProgressRange(uint64_t(size) / 1024u);
      }

      if (got_size)
        env.SetProgressPosition(uint64_t(position) / 1024u);
    }
  }

  void OnDownloadComplete(Path _path_relative) noexcept override {
          LogFormat("OnDownloadComplete");
    if (!complete && path_relative == _path_relative) {
      complete = true;
      success = true;

      download_complete_notify.SendNotification();
      DirtyTaskListPanel();
    }
  }

  void OnDownloadError(Path _path_relative,
                       std::exception_ptr _error) noexcept override {
    if (!complete && path_relative == _path_relative) {
      complete = true;
      success = false;
      error = std::move(_error);
      download_complete_notify.SendNotification();
    }
  }

  void OnDownloadCompleteNotification() noexcept {
    assert(complete);
    dialog.SetModalResult(success ? mrOK : mrCancel);
  }
};

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
  parent.SetCurrent(1);
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
  ExternalLogger::Declare(decl, way_points.GetHome().get());
}

inline void
TaskActionsPanel::OnDownloadClicked() noexcept
{
  try{
    const WeGlideSettings &settings = CommonInterface::GetComputerSettings().weglide;
    char url[256];
    TCHAR filename[256] = _T("");

    snprintf(url, sizeof(url),"https://api.weglide.org/v1/task/declaration/%u?cup=false&tsk=true", settings.pilot_id);

    const NMEAInfo &basic = CommonInterface::Basic();
    const BrokenDateTime t = basic.date_time_utc;

    _stprintf(filename, _T("declared_%04d-%02d-%02d_%02d-%02d.tsk"),t.year,t.month,t.day,t.hour,t.minute);
    const auto we_path = MakeLocalPath(_T("weglide"));
    const auto alloc_path = AllocatedPath::Build(we_path, filename);
    /* Remove allocation, because download manager adds it again */
    const auto path = RelativePath(alloc_path);

    ProgressDialog dialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(), _("Download"));

    dialog.SetText(filename);

    dialog.AddCancelButton();

    const WeGlideDownloadProgress dp(dialog, path);
    Net::DownloadManager::Enqueue(url, path);

    int result = dialog.ShowModal();
    if (result != mrOK) {
      Net::DownloadManager::Cancel(path);
      dp.Rethrow();
    }else{
      char line[256];
                  
      if (File::ReadString(alloc_path, line, sizeof(line))){
        if (strcmp (line,"null") == 0){
          File::Delete(alloc_path);
          ShowMessageBox(_("WeGlide Task Not Declared."), _("Download Error"),
                MB_OK);
        }
        else {
          ShowMessageBox(_("WeGlide Task Downloaded."), _("Download Successfull"),
                MB_OK);
        }
      }
    }
  } catch (const std::runtime_error &e) {
  }
}


void
TaskActionsPanel::ReClick() noexcept
{
  dialog.TaskViewClicked();
}

void
TaskActionsPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  const WeGlideSettings &settings = CommonInterface::GetComputerSettings().weglide;

  AddButton(_("New Task"), [this](){ OnNewTaskClicked(); });
  AddButton(_("Declare"), [this](){ OnDeclareClicked(); });
  AddButton(_("Browse"), [this](){ OnBrowseClicked(); });
  AddButton(_("Save"), [this](){ SaveTask(); });  
  if (settings.pilot_id != 0){
	  AddButton(_("Download WeGlide Declared Task"), [this](){ OnDownloadClicked(); });
  }

  if (is_simulator())
    /* cannot communicate with real devices in simulator mode */
    SetRowEnabled(DECLARE, false);
}
