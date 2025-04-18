// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideTasksPanel.hpp"
#include "Internal.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "ui/event/CoInjectFunction.hpp"
#include "net/client/WeGlide/ListTasks.hpp"
#include "net/client/WeGlide/DownloadTask.hpp"
#include "net/http/Init.hpp"
#include "lib/curl/Global.hxx"
#include "util/Compiler.h"
#include "util/ConvertString.hpp"
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
      return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    LoadTask();
  }
};

void
WeGlideTasksPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                               unsigned idx) noexcept
{
  assert(idx <= list.size());
  const auto &info = list[idx];

  if (UTF8ToWideConverter w{info.name.c_str()}; w.IsValid())
    row_renderer.DrawFirstRow(canvas, rc, w);

  if (selection != WeGlideTaskSelection::USER)
    if (UTF8ToWideConverter w{info.user_name.c_str()}; w.IsValid())
      row_renderer.DrawSecondRow(canvas, rc, w);

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
                        GetList().SetLength(list.size());
                        UpdateButtons();
                      },
                      [](std::exception_ptr error){
                        ShowError(error, _T("Error"));
                      });
}

void
WeGlideTasksPanel::RefreshView() noexcept
{
  //const auto &info = list[GetList().GetCursorIndex()];

  dialog.InvalidateTaskView();

  // TODO dialog.ShowTaskView(...);
  (void)summary; // TODO

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
