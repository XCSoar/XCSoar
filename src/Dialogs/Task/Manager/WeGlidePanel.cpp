// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlidePanel.hpp"
#include "WeGlideTasksPanel.hpp"

#ifdef HAVE_HTTP

#include "Internal.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/http/Init.hpp"
#include "net/client/WeGlide/DownloadTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"

class WeGlideActionsPanel final : public RowFormWidget {
  enum Controls {
    WEGLIDE_STATUS,
    DOWNLOAD_DECLARATION,
    MY_TASKS,
    DECLARED_TASKS,
    COMPETITIONS_TODAY,
    RECENT_SCORES,
  };

  TaskManagerDialog &dialog;
  WeGlidePanel &parent;

  std::unique_ptr<OrderedTask> &active_task;
  bool *task_modified;

public:
  WeGlideActionsPanel(TaskManagerDialog &_dialog, WeGlidePanel &_parent,
                      std::unique_ptr<OrderedTask> &_active_task,
                      bool *_task_modified) noexcept
    :RowFormWidget(_dialog.GetLook()),
     dialog(_dialog), parent(_parent),
     active_task(_active_task), task_modified(_task_modified) {}

  void OnDownloadClicked() noexcept;

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
WeGlideActionsPanel::OnDownloadClicked() noexcept
try {
  const auto &settings = CommonInterface::GetComputerSettings();

  PluggableOperationEnvironment env;

  auto task = ShowCoFunctionDialog(dialog.GetMainWindow(), GetLook(),
                                   _("Download"),
                                   WeGlide::DownloadDeclaredTask(*Net::curl,
                                                                 settings.weglide,
                                                                 settings.task,
                                                                 data_components->waypoints.get(),
                                                                 env),
                                   &env);
  if (!task)
    return;

  if (!*task) {
    ShowMessageBox(_("No task"), _("Error"), MB_OK|MB_ICONEXCLAMATION);
    return;
  }

  active_task = (*task)->Clone(settings.task);
  *task_modified = true;
  dialog.ResetTaskView();

  dialog.SwitchToEditTab();
} catch (const std::runtime_error &e) {
  ShowError(std::current_exception(), _("Download"));
}

void
WeGlideActionsPanel::Prepare([[maybe_unused]] ContainerWindow &_parent,
                             [[maybe_unused]] const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings();

  AddReadOnly(_("WeGlide"),
              nullptr,
              settings.weglide.enabled
              ? _("On") : _("Off"));

  const bool weglide_enabled = settings.weglide.enabled;
  const bool pilot_configured = weglide_enabled &&
    settings.weglide.pilot_id != 0;

  AddButton(_("Download Declaration"),
            [this](){ OnDownloadClicked(); });
  SetRowEnabled(DOWNLOAD_DECLARATION, pilot_configured);

  AddButton(_("My Tasks"), [this](){
    parent.SetCurrent(parent.PAGE_USER);
  });
  SetRowEnabled(MY_TASKS, pilot_configured);

  AddButton(_("Declared Tasks"), [this](){
    parent.SetCurrent(parent.PAGE_PUBLIC_DECLARED);
  });
  SetRowEnabled(DECLARED_TASKS, weglide_enabled);

  AddButton(_("Competitions Today"), [this](){
    parent.SetCurrent(parent.PAGE_DAILY_COMPETITIONS);
  });
  SetRowEnabled(COMPETITIONS_TODAY, weglide_enabled);

  AddButton(_("Recent Scores"), [this](){
    parent.SetCurrent(parent.PAGE_RECENT_SCORES);
  });
  SetRowEnabled(RECENT_SCORES, weglide_enabled);
}

WeGlidePanel::WeGlidePanel(TaskManagerDialog &dialog,
                           std::unique_ptr<OrderedTask> &_active_task,
                           bool *_task_modified) noexcept
{
  Add(std::make_unique<WeGlideActionsPanel>(dialog, *this,
                                            _active_task, _task_modified));
  Add(CreateWeGlideTasksPanel(dialog, WeGlideTaskSelection::USER,
                              _active_task, _task_modified));
  Add(CreateWeGlideTasksPanel(dialog, WeGlideTaskSelection::PUBLIC_DECLARED,
                              _active_task, _task_modified));
  Add(CreateWeGlideTasksPanel(dialog, WeGlideTaskSelection::DAILY_COMPETITIONS,
                              _active_task, _task_modified));
  Add(CreateWeGlideTasksPanel(dialog, WeGlideTaskSelection::RECENT_SCORES,
                              _active_task, _task_modified));
}

void
WeGlidePanel::ReClick() noexcept
{
  if (GetCurrentIndex() > 0)
    SetCurrent(PAGE_ACTIONS);
  else
    PagerWidget::ReClick();
}

void
WeGlidePanel::Show(const PixelRect &rc) noexcept
{
  SetCurrent(PAGE_ACTIONS);
  PagerWidget::Show(rc);
}

#endif
