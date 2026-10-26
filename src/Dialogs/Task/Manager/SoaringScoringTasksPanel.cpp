// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SoaringScoringTasksPanel.hpp"
#include "Language/Language.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_HTTP

#include "Internal.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Look/DialogLook.hpp"
#include "Operation/Operation.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "UIGlobals.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "lib/curl/Global.hxx"
#include "net/client/SoaringScoring/Account.hpp"
#include "net/client/SoaringScoring/Client.hpp"
#include "net/http/Init.hpp"
#include "ui/event/CoInjectFunction.hpp"
#include "util/StaticString.hxx"

#include <cassert>

class SoaringScoringTasksPanel final : public ListWidget {
  TaskManagerDialog &dialog;
  TwoTextRowsRenderer row_renderer;

  std::unique_ptr<OrderedTask> &active_task;
  bool *task_modified;

  ButtonPanelWidget *buttons = nullptr;
  Button *load_button = nullptr;

  SoaringScoring::FlightTaskList tasks;
  bool several_contests = false;

  NullOperationEnvironment null_progress;
  UI::CoInjectFunction<SoaringScoring::FlightTaskList> inject_reload{
    Net::curl->GetEventLoop()};

public:
  SoaringScoringTasksPanel(TaskManagerDialog &_dialog,
                           std::unique_ptr<OrderedTask> &_active_task,
                           bool *_task_modified) noexcept
    :dialog(_dialog),
     active_task(_active_task),
     task_modified(_task_modified) {}

  ~SoaringScoringTasksPanel() noexcept override {
    inject_reload.Cancel();
  }

  void SetButtonPanel(ButtonPanelWidget &_buttons) noexcept {
    buttons = &_buttons;
  }

  void CreateButtons(ButtonPanel &panel) noexcept {
    load_button = panel.Add(_("Load"), [this]() { LoadTask(); });
    panel.Add(_("Refresh"), [this]() { ReloadList(); });
    panel.EnableCursorSelection();
  }

  void UpdateButtons() noexcept {
    if (load_button != nullptr)
      load_button->SetEnabled(!tasks.tasks.empty());
  }

  void ReloadList() noexcept;
  void LoadTask() noexcept;

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  bool CanActivateItem(unsigned) const noexcept override {
    return !tasks.tasks.empty();
  }

  void OnActivateItem(unsigned) noexcept override {
    if (!tasks.tasks.empty())
      LoadTask();
  }
};

void
SoaringScoringTasksPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                     unsigned idx) noexcept
{
  if (tasks.tasks.empty()) {
    row_renderer.DrawFirstRow(canvas, rc,
                              SoaringScoring::AccountConnected()
                                ? _("No tasks")
                                : _("Sign in from Services, SoaringScoring."));
    return;
  }

  assert(idx < tasks.tasks.size());
  const auto &task = tasks.tasks[idx];

  StaticString<256> title;
  if (several_contests && !task.contest_name.empty())
    title.Format("%s: %s", task.contest_name.c_str(),
                 task.row.display_label.c_str());
  else
    title = task.row.display_label.c_str();
  row_renderer.DrawFirstRow(canvas, rc, title.c_str());

  StaticString<256> second;
  second = task.row.class_name.c_str();
  if (task.row.has_distance) {
    char distance[32];
    FormatUserDistance(task.row.nominal_distance_km * 1000, distance,
                       true, 0);
    if (!second.empty())
      second.append("  ");
    second.append(distance);
  }
  if (task.row.has_handicap) {
    const unsigned handicap =
      SoaringScoring::HandicapIndex(task.row.handicap);
    if (handicap != 0) {
      if (!second.empty())
        second.append("  ");
      second.AppendFormat("%s %u", _("Handicap"), handicap);
    }
  }
  if (!second.empty())
    row_renderer.DrawSecondRow(canvas, rc, second.c_str());
}

void
SoaringScoringTasksPanel::ReloadList() noexcept
{
  if (Net::curl == nullptr || !SoaringScoring::AccountConnected()) {
    tasks = {};
    several_contests = false;
    GetList().SetLength(1);
    UpdateButtons();
    return;
  }

  const unsigned handicap =
    CommonInterface::GetComputerSettings().contest.handicap;

  inject_reload.Cancel();
  const auto token = SoaringScoring::LoadAccountToken().access_token;
  inject_reload.Start(
    SoaringScoring::LoadFlightTasks(*Net::curl, token, handicap,
                                    null_progress),
    [this](SoaringScoring::FlightTaskList &&list) {
      tasks = std::move(list);
      several_contests = false;
      if (!tasks.tasks.empty()) {
        const auto &name = tasks.tasks.front().contest_name;
        for (const auto &task : tasks.tasks) {
          if (task.contest_name != name) {
            several_contests = true;
            break;
          }
        }
      }
      GetList().SetLength(tasks.tasks.empty() ? 1 : tasks.tasks.size());
      if (!tasks.tasks.empty())
        GetList().SetCursorIndex(tasks.suggested);
      UpdateButtons();
    },
    [this](std::exception_ptr error) {
      ShowError(error, _("SoaringScoring"));
    });
}

void
SoaringScoringTasksPanel::LoadTask() noexcept
try {
  if (tasks.tasks.empty() || Net::curl == nullptr)
    return;

  const unsigned index = GetList().GetCursorIndex();
  if (index >= tasks.tasks.size())
    return;

  const auto &selected = tasks.tasks[index];
  PluggableOperationEnvironment env;
  const auto &settings = CommonInterface::GetComputerSettings();
  const Waypoints *waypoints = data_components != nullptr
                                 ? data_components->waypoints.get()
                                 : nullptr;
  auto task = ShowCoFunctionDialog(
    dialog.GetMainWindow(), UIGlobals::GetDialogLook(), _("Download"),
    SoaringScoring::DownloadFlightTask(
      *Net::curl, SoaringScoring::LoadAccountToken().access_token,
      selected.row.xcsoar_tsk_url, settings.task, waypoints, env),
    &env);
  if (!task || !*task)
    return;

  active_task = (*task)->Clone(settings.task);
  *task_modified = true;
  dialog.ResetTaskView();
  dialog.SwitchToEditTab();
} catch (const std::exception &) {
  ShowError(std::current_exception(), _("Download"));
}

void
SoaringScoringTasksPanel::Prepare(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, dialog.GetLook(), rc,
             row_renderer.CalculateLayout(*look.list.font,
                                          look.small_font));
  if (buttons != nullptr)
    CreateButtons(buttons->GetButtonPanel());
}

void
SoaringScoringTasksPanel::Show(const PixelRect &rc) noexcept
{
  ReloadList();
  UpdateButtons();
  ListWidget::Show(rc);
}

void
SoaringScoringTasksPanel::Hide() noexcept
{
  inject_reload.Cancel();
  ListWidget::Hide();
}

std::unique_ptr<Widget>
CreateSoaringScoringTasksPanel(TaskManagerDialog &dialog,
                               std::unique_ptr<OrderedTask> &active_task,
                               bool *task_modified) noexcept
{
  auto widget = std::make_unique<SoaringScoringTasksPanel>(
    dialog, active_task, task_modified);
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(widget), ButtonPanelWidget::Alignment::BOTTOM);
  auto &list = (SoaringScoringTasksPanel &)buttons->GetWidget();
  list.SetButtonPanel(*buttons);
  return buttons;
}

#else

#include "Language/FormatText.hpp"
#include "Widget/TextWidget.hpp"
#include "util/StaticString.hxx"

std::unique_ptr<Widget>
CreateSoaringScoringTasksPanel(TaskManagerDialog &,
                               std::unique_ptr<OrderedTask> &,
                               bool *) noexcept
{
  auto widget = std::make_unique<TextWidget>();
  StaticString<128> message;
  FormatFeatureNotAvailableInThisBuild(message, "SoaringScoring");
  widget->SetText(message);
  return widget;
}

#endif
