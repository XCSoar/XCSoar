// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideTasksPanel.hpp"
#include "Widget/TextWidget.hpp"
#include "Language/Language.hpp"

#ifdef HAVE_HTTP
#include "Internal.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "util/StaticString.hxx"
#include "Look/DialogLook.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "ui/event/CoInjectFunction.hpp"
#include "net/client/WeGlide/ListTasks.hpp"
#include "net/client/WeGlide/DownloadTask.hpp"
#include "net/http/Init.hpp"
#include "lib/curl/Global.hxx"
#include "util/Compiler.h"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Interface.hpp"

#include <cassert>

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class WeGlideTasksPanel final
  : public ListWidget {

  TaskManagerDialog &dialog;

  TwoTextRowsRenderer row_renderer;

  std::unique_ptr<OrderedTask> &active_task;
  bool *task_modified;

  TextWidget &summary;
  TwoWidgets *two_widgets;
  ButtonPanelWidget *buttons;
  Button *load_button;

  using List = std::vector<WeGlide::TaskInfo>;
  List list;

  NullOperationEnvironment null_progress_listener;
  UI::CoInjectFunction<List> inject_reload{Net::curl->GetEventLoop()};

  const WeGlideTaskSelection selection;

public:
  WeGlideTasksPanel(TaskManagerDialog &_dialog,
                    WeGlideTaskSelection _selection,
                    std::unique_ptr<OrderedTask> &_active_task, bool *_task_modified,
                    TextWidget &_summary) noexcept
    :dialog(_dialog),
     active_task(_active_task), task_modified(_task_modified),
     summary(_summary),
     selection(_selection) {}

  void SetTwoWidgets(TwoWidgets &_two_widgets) noexcept {
    two_widgets = &_two_widgets;
  }

  void SetButtonPanel(ButtonPanelWidget &_buttons) noexcept {
    buttons = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons) noexcept {
    load_button = buttons.Add(_("Load"), [this](){ LoadTask(); });
    buttons.Add(_("Refresh"), [this](){ ReloadList(); });
  }

  void UpdateButtons() noexcept {
    load_button->SetEnabled(!list.empty());
  }

  void ReloadList() noexcept;

  void RefreshView() noexcept;

  void LoadTask() noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  /* virtual methods from class ListControl::Handler */
  void OnPaintItem([[maybe_unused]] Canvas &canvas, [[maybe_unused]] const PixelRect rc,
                   [[maybe_unused]] unsigned idx) noexcept override;

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    RefreshView();
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
      return !list.empty();
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    if (!list.empty())
      LoadTask();
  }
};

void
WeGlideTasksPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                               unsigned idx) noexcept
{
  if (list.empty()) {
    row_renderer.DrawFirstRow(canvas, rc, _("No tasks"));
    return;
  }

  assert(idx < list.size());
  const auto &info = list[idx];

  if (info.name.c_str())
    row_renderer.DrawFirstRow(canvas, rc, info.name.c_str());

  StaticString<256> second_row;
  second_row.clear();

  if (!info.scoring_date.empty()) {
    second_row.append(info.scoring_date.c_str());
  }

  if (info.kind != WeGlide::TaskKind::UNKNOWN) {
    if (!second_row.empty())
      second_row.append(", ");
    second_row.append(WeGlide::ToString(info.kind));
  }

  if (!info.turnpoints.empty()) {
    if (!second_row.empty())
      second_row.append(", ");
    second_row.AppendFormat("%u TPs", (unsigned)info.turnpoints.size());
  }

  if (selection != WeGlideTaskSelection::USER && !info.user_name.empty()) {
    if (!second_row.empty())
      second_row.append(" · ");
    second_row.append(info.user_name.c_str());
  }

  if (!info.ruleset.empty()) {
    if (!second_row.empty())
      second_row.append(" · ");
    second_row.append(info.ruleset.c_str());
  }

  if (!second_row.empty())
    row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());

  row_renderer.DrawRightSecondRow(canvas, rc,
                                  FormatUserDistanceSmart(info.distance));
}

static auto
LoadTaskList(WeGlideTaskSelection selection,
             const WeGlideSettings &settings,
             ProgressListener &progress) noexcept
{
  switch (selection) {
  case WeGlideTaskSelection::USER:
    return WeGlide::ListTasksByUser(*Net::curl, settings,
                                    settings.pilot_id,
                                    progress);

  case WeGlideTaskSelection::PUBLIC_DECLARED:
    return WeGlide::ListDeclaredTasks(*Net::curl, settings,
                                      progress);

  case WeGlideTaskSelection::DAILY_COMPETITIONS:
    return WeGlide::ListDailyCompetitions(*Net::curl, settings,
                                          progress);

  case WeGlideTaskSelection::RECENT_SCORES:
    return WeGlide::ListRecentTaskScores(*Net::curl, settings,
                                         progress);
  }

  gcc_unreachable();
}

inline void
WeGlideTasksPanel::ReloadList() noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings();

  inject_reload.Cancel();
  inject_reload.Start(LoadTaskList(selection, settings.weglide,
                                   null_progress_listener),
                      [this](List &&_list){
                        list = std::move(_list);
                        GetList().SetLength(list.empty() ? 1 : list.size());
                        UpdateButtons();
                        RefreshView();
                      },
                      [this](std::exception_ptr error){
                        ShowError(error, "Error");
                        RefreshView();
                      });
}

void
WeGlideTasksPanel::RefreshView() noexcept
{
  dialog.InvalidateTaskView();

  if (list.empty() || GetList().GetCursorIndex() >= list.size()) {
    summary.SetText("");
  } else {
    const auto &info = list[GetList().GetCursorIndex()];

    StaticString<512> text;
    text.clear();

    if (!info.scores.empty()) {
      unsigned rank = 1;
      for (const auto &se : info.scores) {
        if (!text.empty())
          text.append("\n");
        text.AppendFormat("%u. %s — %.0f pts, %.1f km/h",
                         rank++,
                         se.user_name.c_str(),
                         se.points, se.speed);
      }
    } else if (!info.turnpoints.empty()) {
      for (const auto &tp : info.turnpoints) {
        if (!text.empty())
          text.append(" → ");
        text.append(tp.name.c_str());
      }
    }

    summary.SetText(text.c_str());
  }

  if (GetList().IsVisible() && two_widgets != nullptr)
    two_widgets->UpdateLayout();
}

inline void
WeGlideTasksPanel::LoadTask() noexcept
try {
  const DialogLook &look = UIGlobals::GetDialogLook();
  const auto &settings = CommonInterface::GetComputerSettings();
  const auto &info = list[GetList().GetCursorIndex()];

  PluggableOperationEnvironment env;

  auto task = ShowCoFunctionDialog(dialog.GetMainWindow(), look,
                                   _("Download"),
                                   WeGlide::DownloadTask(*Net::curl,
                                                         settings.weglide,
                                                         info.id,
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
WeGlideTasksPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  CreateList(parent, dialog.GetLook(), rc,
             row_renderer.CalculateLayout(*look.list.font,
                                          look.small_font));

  CreateButtons(buttons->GetButtonPanel());
}

void
WeGlideTasksPanel::Show(const PixelRect &rc) noexcept
{
  // TODO dialog.ShowTaskView(get_cursor_task());

  RefreshView();
  UpdateButtons();
  ReloadList();

  ListWidget::Show(rc);
}

void
WeGlideTasksPanel::Hide() noexcept
{
  inject_reload.Cancel();
  ListWidget::Hide();
}

std::unique_ptr<Widget>
CreateWeGlideTasksPanel(TaskManagerDialog &dialog,
                        WeGlideTaskSelection selection,
                        std::unique_ptr<OrderedTask> &active_task,
                        bool *task_modified) noexcept
{
  auto summary = std::make_unique<TextWidget>();
  auto widget = std::make_unique<WeGlideTasksPanel>(dialog, selection,
                                                    active_task, task_modified,
                                                    *summary);
  auto tw = std::make_unique<TwoWidgets>(std::move(widget),
                                         std::move(summary));
  auto &list = (WeGlideTasksPanel &)tw->GetFirst();

  list.SetTwoWidgets(*tw);

  auto buttons =
    std::make_unique<ButtonPanelWidget>(std::move(tw),
                                        ButtonPanelWidget::Alignment::BOTTOM);
  list.SetButtonPanel(*buttons);

  return buttons;
}

#else

std::unique_ptr<Widget>
CreateWeGlideTasksPanel([[maybe_unused]] TaskManagerDialog &dialog,
                        [[maybe_unused]] WeGlideTaskSelection selection,
                        [[maybe_unused]]
                         std::unique_ptr<OrderedTask> &active_task,
                        [[maybe_unused]] bool *task_modified) noexcept
{
  auto widget = std::make_unique<TextWidget>();
  widget->SetText(_("WeGlide is not available in this build."));
  return widget;
}

#endif
