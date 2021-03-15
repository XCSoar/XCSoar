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

#include "TaskEditPanel.hpp"
#include "Internal.hpp"
#include "../TaskDialogs.hpp"
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Interface.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/List.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Look/DialogLook.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/IntermediatePoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Language/Language.hpp"
#include "Renderer/OZPreviewRenderer.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"
#include "UIGlobals.hpp"

#include <cassert>

class TaskEditPanel
  : public ListWidget {
  TaskManagerDialog &dialog;

  const TaskLook &task_look;
  const AirspaceLook &airspace_look;

  std::unique_ptr<OrderedTask> &ordered_task_pointer;
  OrderedTask *ordered_task;
  bool *task_modified;

  TextWidget &summary;

  ButtonPanelWidget *buttons_widget;
  Button *edit_button, *mutate_button;
  Button *down_button, *up_button;
  Button *reverse_button, *clear_all_button;

  TwoWidgets *two_widgets;

  TwoTextRowsRenderer row_renderer;

public:
  TaskEditPanel(TaskManagerDialog &_dialog,
                const TaskLook &_task_look, const AirspaceLook &_airspace_look,
                std::unique_ptr<OrderedTask> &_active_task, bool *_task_modified,
                TextWidget &_summary) noexcept
    :dialog(_dialog),
     task_look(_task_look), airspace_look(_airspace_look),
     ordered_task_pointer(_active_task), task_modified(_task_modified),
     summary(_summary), two_widgets(nullptr) {}

  void SetTwoWidgets(TwoWidgets &_two_widgets) {
    two_widgets = &_two_widgets;
  }

  void SetButtons(ButtonPanelWidget &_buttons) noexcept {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons) noexcept;

  void UpdateButtons();

  void MoveUp();
  void MoveDown();

  void ReverseTask();
  void OnClearAllClicked();
  void OnEditTurnpointClicked();
  void EditTaskPoint(unsigned ItemIndex);
  void OnMakeFinish();

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;

protected:
  void RefreshView();

private:
  /* virtual methods from List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;
  void OnCursorMoved(unsigned index) noexcept override;
  bool CanActivateItem(unsigned index) const noexcept override;
  void OnActivateItem(unsigned index) noexcept override;
};

void
TaskEditPanel::CreateButtons(ButtonPanel &buttons) noexcept
{
  edit_button = buttons.Add(_("Edit Point"),
                            [this](){ OnEditTurnpointClicked(); });
  mutate_button = buttons.Add(_("Make Finish"),
                              [this](){ OnMakeFinish(); });
  down_button = buttons.Add(std::make_unique<SymbolButtonRenderer>(buttons.GetLook(), _T("v")),
                            [this](){ MoveDown(); });
  up_button = buttons.Add(std::make_unique<SymbolButtonRenderer>(buttons.GetLook(), _T("^")),
                          [this](){ MoveUp(); });
  reverse_button = buttons.Add(_("Reverse"),
                               [this](){ ReverseTask(); });
  clear_all_button = buttons.Add(_("Clear All"),
                                 [this](){ OnClearAllClicked(); });

  buttons.EnableCursorSelection();
}

void
TaskEditPanel::UpdateButtons()
{
  const unsigned index = GetList().GetCursorIndex();

  edit_button->SetVisible(index < ordered_task->TaskSize());
  mutate_button->SetVisible(index > 0 &&
                            (index == ordered_task->TaskSize() - 1) &&
                            !ordered_task->HasFinish());
  down_button->SetVisible((int)index < ((int)(ordered_task->TaskSize()) - 1));
  up_button->SetVisible(index > 0 && index < ordered_task->TaskSize());
  reverse_button->SetVisible(ordered_task->TaskSize() >= 2);
}

void
TaskEditPanel::RefreshView()
{
  dialog.InvalidateTaskView();

  unsigned length = ordered_task->TaskSize();
  if (!ordered_task->IsFull())
    ++length;
  GetList().SetLength(length);
  GetList().Invalidate();

  UpdateButtons();

  {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text, false);
    summary.SetText(text);
  }

  if (GetList().IsVisible() && two_widgets != nullptr)
    two_widgets->UpdateLayout();
}

void TaskEditPanel::ReverseTask()
{
  if (ordered_task->TaskSize() < 2)
    return;

  const unsigned start_index = 0;
  const unsigned finish_index = ordered_task->TaskSize() - 1;
  auto start_wp = ordered_task->GetTaskPoint(start_index).GetWaypointPtr();
  auto finish_wp = ordered_task->GetTaskPoint(finish_index).GetWaypointPtr();

  if (start_wp->location != finish_wp->location) {
    // swap start/finish TP if at different location but leave OZ type intact
    ordered_task->Relocate(start_index, std::move(finish_wp));
    ordered_task->Relocate(finish_index, std::move(start_wp));

    // remove optional start points
    while (ordered_task->HasOptionalStarts())
      ordered_task->RemoveOptionalStart(0);
  }

  // reverse intermediate TPs order keeping the OZ type with the respective TP
  unsigned length = ordered_task->TaskSize()-1;
  for (unsigned i = 1; i < length - 1; ++i) {
    const OrderedTaskPoint &otp = ordered_task->GetTaskPoint(length - 1);
    if (!ordered_task->GetFactory().Insert(otp, i, false))
      return;
    if (!ordered_task->GetFactory().Remove(length, false))
      return;
  }

  *task_modified = true;
  ordered_task->ClearName();
  ordered_task->GetFactory().CheckAddFinish();
  ordered_task->UpdateStatsGeometry();
  ordered_task->UpdateGeometry();
  RefreshView();
}

void
TaskEditPanel::OnClearAllClicked()
{
  if ((ordered_task->TaskSize() < 2) ||
      (ShowMessageBox(_("Clear all points?"), _("Task edit"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {

    ordered_task->RemoveAllPoints();
    ordered_task->ClearName();
    *task_modified = true;
    RefreshView();
  }
}

void
TaskEditPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned DrawListIndex) noexcept
{
  assert(DrawListIndex <= ordered_task->TaskSize());

  const unsigned padding = Layout::GetTextPadding();
  const unsigned line_height = rc.GetHeight();

  TCHAR buffer[120];

  // Draw "Add turnpoint" label
  if (DrawListIndex == ordered_task->TaskSize()) {
    row_renderer.DrawFirstRow(canvas, rc, _("Add Turnpoint"));
    return;
  }

  const OrderedTaskPoint &tp = ordered_task->GetTaskPoint(DrawListIndex);
  GeoVector leg = tp.GetNominalLegVector();
  bool show_leg_info = leg.distance > 0.01;

  PixelRect text_rc = rc;
  text_rc.left += line_height + padding;

  if (show_leg_info) {
    // Draw leg distance
    FormatUserDistanceSmart(leg.distance, buffer, true);
    const int x1 = row_renderer.DrawRightFirstRow(canvas, rc, buffer);

    // Draw leg bearing
    FormatBearing(buffer, ARRAY_SIZE(buffer), leg.bearing);
    const int x2 = row_renderer.DrawRightSecondRow(canvas, rc, buffer);

    text_rc.right = std::min(x1, x2);
  }

  // Draw details line
  OrderedTaskPointRadiusLabel(tp.GetObservationZone(), buffer);
  if (!StringIsEmpty(buffer))
    row_renderer.DrawSecondRow(canvas, text_rc, buffer);

  // Draw turnpoint name
  OrderedTaskPointLabel(tp.GetType(), tp.GetWaypointPtr()->name.c_str(),
                        DrawListIndex, buffer);
  row_renderer.DrawFirstRow(canvas, text_rc, buffer);

  // Draw icon
  const PixelPoint pt(rc.left + line_height / 2,
                      rc.top + line_height / 2);

  const unsigned radius = line_height / 2 - padding;
  OZPreviewRenderer::Draw(canvas, tp.GetObservationZone(),
                          pt, radius, task_look,
                          CommonInterface::GetMapSettings().airspace,
                          airspace_look);
}

void
TaskEditPanel::OnEditTurnpointClicked()
{
  EditTaskPoint(GetList().GetCursorIndex());
}

bool
TaskEditPanel::CanActivateItem(gcc_unused unsigned index) const noexcept
{
  return true;
}

void
TaskEditPanel::EditTaskPoint(unsigned ItemIndex)
{
  if (ItemIndex < ordered_task->TaskSize()) {
    if (dlgTaskPointShowModal(*ordered_task, ItemIndex)) {
      *task_modified = true;
      ordered_task->ClearName();
      ordered_task->UpdateGeometry();
      RefreshView();
    }
  } else if (!ordered_task->IsFull()) {

    AbstractTaskFactory &factory = ordered_task->GetFactory();
    auto way_point =
      ShowWaypointListDialog(ordered_task->TaskSize() > 0
                             ? ordered_task->GetPoint(ordered_task->TaskSize() - 1).GetLocation()
                             : CommonInterface::Basic().location,
                        ordered_task, ItemIndex);
    if (!way_point)
      return;

    auto point = ItemIndex == 0
      ? (std::unique_ptr<OrderedTaskPoint>)factory.CreateStart(std::move(way_point))
      : (std::unique_ptr<OrderedTaskPoint>)factory.CreateIntermediate(std::move(way_point));
    if (point == nullptr)
      return;

    if (factory.Append(*point, true)) {
      *task_modified = true;
      ordered_task->ClearName();
      ordered_task->UpdateGeometry();
      RefreshView();
    }
  }
}

void
TaskEditPanel::OnActivateItem(unsigned index) noexcept
{
  EditTaskPoint(index);
}

void
TaskEditPanel::OnCursorMoved(gcc_unused unsigned index) noexcept
{
  UpdateButtons();
}

void
TaskEditPanel::OnMakeFinish()
{
  ordered_task->UpdateStatsGeometry();
  if (ordered_task->GetFactory().CheckAddFinish())
    ordered_task->UpdateGeometry();

  RefreshView();
}

void
TaskEditPanel::MoveUp()
{
  unsigned index = GetList().GetCursorIndex();
  if (index == 0)
    return;

  if (!ordered_task->GetFactory().Swap(index - 1, true))
    return;

  GetList().SetCursorIndex(index - 1);
  *task_modified = true;
  ordered_task->ClearName();

  ordered_task->UpdateGeometry();
  RefreshView();
}

void
TaskEditPanel::MoveDown()
{
  unsigned index = GetList().GetCursorIndex();
  if (index >= ordered_task->TaskSize())
    return;

  if (!ordered_task->GetFactory().Swap(index, true))
    return;

  GetList().SetCursorIndex(index + 1);
  *task_modified = true;
  ordered_task->ClearName();

  ordered_task->UpdateGeometry();
  RefreshView();
}

void
TaskEditPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));

  CreateButtons(buttons_widget->GetButtonPanel());

  ordered_task = ordered_task_pointer.get();
}

void
TaskEditPanel::ReClick() noexcept
{
  dialog.TaskViewClicked();
}

void
TaskEditPanel::Show(const PixelRect &rc) noexcept
{
  if (ordered_task != ordered_task_pointer.get()) {
    ordered_task = ordered_task_pointer.get();
    GetList().SetCursorIndex(0);
  }

  RefreshView();

  ListWidget::Show(rc);
}

std::unique_ptr<Widget>
CreateTaskEditPanel(TaskManagerDialog &dialog,
                    const TaskLook &task_look,
                    const AirspaceLook &airspace_look,
                    std::unique_ptr<OrderedTask> &active_task,
                    bool *task_modified) noexcept
{
  auto summary = std::make_unique<TextWidget>();
  auto widget = std::make_unique<TaskEditPanel>(dialog, task_look, airspace_look,
                                                active_task, task_modified,
                                                *summary);
  auto tw1 = std::make_unique<TwoWidgets>(std::move(widget),
                                          std::move(summary));

  auto &w = (TaskEditPanel &)tw1->GetFirst();
  w.SetTwoWidgets(*tw1);

  auto buttons = std::make_unique<ButtonPanelWidget>(std::move(tw1),
                                                     ButtonPanelWidget::Alignment::BOTTOM);

  w.SetButtons(*buttons);

  return buttons;
}
