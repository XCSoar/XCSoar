/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Interface.hpp"
#include "Screen/SingleWindow.hpp"
#include "Form/Button.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/List.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Look/DialogLook.hpp"
#include "Look/TaskLook.hpp"
#include "Look/AirspaceLook.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Language/Language.hpp"
#include "Renderer/OZPreviewRenderer.hpp"
#include "Util/Macros.hpp"
#include "UIGlobals.hpp"

#include <assert.h>
#include <stdio.h>

enum Buttons {
  EDIT = 100,
  MUTATE,
  DOWN,
  UP,
  REVERSE,
  CLEAR_ALL,
};

/**
 * The bottom panel showing four buttons.  Internally, there are five
 * button objects, because the "down" and the "mutate" buttons are
 * exclusive.
 */
class TaskEditButtons final : public NullWidget {
  ActionListener *listener;

  WndButton *edit_button, *mutate_button;
  WndSymbolButton *down_button, *up_button;
  WndButton *reverse_button, *clear_all_button;
  bool visible;

  bool show_edit, show_mutate, show_down, show_up, show_reverse;

  struct Layout {
    PixelRect edit, down, up, reverse, clear_all;
  };

public:
  TaskEditButtons()
    :visible(false), show_edit(false), show_mutate(false),
     show_down(false), show_up(false), show_reverse(false) {}

  void SetListener(ActionListener &_listener) {
    assert(!visible);

    listener = &_listener;
  }

  void Update(bool _edit, bool _mutate, bool _down, bool _up, bool _reverse) {
    show_edit = _edit;
    show_mutate = _mutate;
    show_down = _down;
    show_up = _up;
    show_reverse = _reverse;

    if (visible)
      UpdateVisibility();
  }

private:
  void UpdateVisibility() {
    assert(visible);

    edit_button->SetVisible(show_edit);
    mutate_button->SetVisible(show_mutate);
    down_button->SetVisible(show_down);
    up_button->SetVisible(show_up);
    reverse_button->SetVisible(show_reverse);
    clear_all_button->Show();
  }

  Layout CalculateLayout(const PixelRect &rc) const {
    const PixelScalar dx = (rc.right - rc.left) / 5;

    return {
      { rc.left         , rc.top, rc.left +     dx, rc.bottom },
      { rc.left +     dx, rc.top, rc.left + 2 * dx, rc.bottom },
      { rc.left + 2 * dx, rc.top, rc.left + 3 * dx, rc.bottom },
      { rc.left + 3 * dx, rc.top, rc.left + 4 * dx, rc.bottom },
      { rc.left + 4 * dx, rc.top, rc.right        , rc.bottom },
    };
  }

  void UpdatePositions(const PixelRect &rc) {
    assert(visible);

    const Layout layout = CalculateLayout(rc);

    edit_button->Move(layout.edit);
    mutate_button->Move(layout.down);
    down_button->Move(layout.down);
    up_button->Move(layout.up);
    reverse_button->Move(layout.reverse);
    clear_all_button->Move(layout.clear_all);
  }

public:
  /* virtual methods from Widget */
  virtual PixelSize GetMinimumSize() const override {
    return { ::Layout::Scale(180),
        PixelScalar(::Layout::GetMinimumControlHeight()) };
  }

  virtual PixelSize GetMaximumSize() const override {
    return { ::Layout::Scale(400),
        PixelScalar(::Layout::GetMaximumControlHeight()) };
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    assert(!visible);

    ButtonWindowStyle style;
    style.Hide();
    style.TabStop();

    const ButtonLook &look = UIGlobals::GetDialogLook().button;

    const Layout layout = CalculateLayout(rc);
    edit_button = new WndButton(parent, look, _("Edit Point"),
                                layout.edit, style,
                                *listener, EDIT);
    mutate_button = new WndButton(parent, look, _("Make Finish"),
                                  layout.down, style,
                                  *listener, MUTATE);
    down_button = new WndSymbolButton(parent, look, _T("v"),
                                      layout.down, style,
                                      *listener, DOWN);
    up_button = new WndSymbolButton(parent, look, _T("^"),
                                    layout.down, style,
                                    *listener, UP);
    reverse_button = new WndButton(parent, look, _("Reverse"),
                                   layout.reverse, style,
                                   *listener, REVERSE);
    clear_all_button = new WndButton(parent, look, _("Clear All"),
                                     layout.clear_all, style,
                                     *listener, CLEAR_ALL);
  }

  virtual void Unprepare() override {
    assert(!visible);

    delete clear_all_button;
    delete reverse_button;
    delete down_button;
    delete up_button;
    delete mutate_button;
    delete edit_button;
  }

  virtual void Show(const PixelRect &rc) override {
    assert(!visible);
    visible = true;

    UpdatePositions(rc);
    UpdateVisibility();
  }

  virtual void Hide() override {
    assert(visible);
    visible = false;

    edit_button->Hide();
    mutate_button->Hide();
    down_button->Hide();
    up_button->Hide();
    reverse_button->Hide();
    clear_all_button->Hide();
  }

  virtual void Move(const PixelRect &rc) override {
    UpdatePositions(rc);
  }
};

class TaskEditPanel
  : public ListWidget, public ActionListener {
  TaskManagerDialog &dialog;

  const TaskLook &task_look;
  const AirspaceLook &airspace_look;

  OrderedTask **ordered_task_pointer, *ordered_task;
  bool *task_modified;

  TextWidget &summary;
  TaskEditButtons &buttons;

  TwoWidgets *two_widgets;

public:
  TaskEditPanel(TaskManagerDialog &_dialog,
                const TaskLook &_task_look, const AirspaceLook &_airspace_look,
                OrderedTask **_active_task, bool *_task_modified,
                TextWidget &_summary, TaskEditButtons &_buttons)
    :dialog(_dialog),
     task_look(_task_look), airspace_look(_airspace_look),
     ordered_task_pointer(_active_task), task_modified(_task_modified),
     summary(_summary), buttons(_buttons), two_widgets(nullptr) {}

  void SetTwoWidgets(TwoWidgets &_two_widgets) {
    two_widgets = &_two_widgets;
  }

  void UpdateButtons();

  void MoveUp();
  void MoveDown();

  void ReverseTask();
  void OnClearAllClicked();
  void OnEditTurnpointClicked();
  void EditTaskPoint(unsigned ItemIndex);
  void OnMakeFinish();

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

  virtual void Unprepare() override {
    DeleteWindow();
  }

  virtual void ReClick() override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;
  virtual bool KeyPress(unsigned key_code) override;

protected:
  void RefreshView();

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

  /* virtual methods from List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;
  virtual void OnCursorMoved(unsigned index) override;
  virtual bool CanActivateItem(unsigned index) const override;
  virtual void OnActivateItem(unsigned index) override;
};

void
TaskEditPanel::UpdateButtons()
{
  const unsigned index = GetList().GetCursorIndex();

  buttons.Update(index < ordered_task->TaskSize(),
                 index > 0 &&
                 (index == ordered_task->TaskSize() - 1) &&
                 !ordered_task->HasFinish(),
                 (int)index < ((int)(ordered_task->TaskSize()) - 1),
                 index > 0 && index < ordered_task->TaskSize(),
                 ordered_task->TaskSize() >= 2);
}

void
TaskEditPanel::RefreshView()
{
  UpdateButtons();

  dialog.InvalidateTaskView();

  unsigned length = ordered_task->TaskSize();
  if (!ordered_task->IsFull())
    ++length;
  GetList().SetLength(length);
  GetList().Invalidate();

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
  const Waypoint start_wp = ordered_task->GetTaskPoint(start_index).GetWaypoint();
  const Waypoint finish_wp = ordered_task->GetTaskPoint(finish_index).GetWaypoint();

  if (start_wp.location != finish_wp.location) {
    // swap start/finish TP if at different location but leave OZ type intact
    ordered_task->Relocate(start_index, finish_wp);
    ordered_task->Relocate(finish_index, start_wp);

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
TaskEditPanel::OnAction(int id)
{
  switch (id) {
  case EDIT:
    OnEditTurnpointClicked();
    break;

  case MUTATE:
    OnMakeFinish();
    break;

  case UP:
    MoveUp();
    break;

  case DOWN:
    MoveDown();
    break;

  case REVERSE:
    ReverseTask();
    break;

  case CLEAR_ALL:
    OnClearAllClicked();
    break;
  }
}

void
TaskEditPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned DrawListIndex)
{
  assert(DrawListIndex <= ordered_task->TaskSize());

  const unsigned padding = Layout::GetTextPadding();
  const PixelScalar line_height = rc.bottom - rc.top;

  TCHAR buffer[120];

  const Font &name_font = *dialog.GetLook().list.font_bold;
  const Font &small_font = *dialog.GetLook().small_font;

  // Draw "Add turnpoint" label
  if (DrawListIndex == ordered_task->TaskSize()) {
    canvas.Select(name_font);
    _stprintf(buffer, _T("  (%s)"), _("Add Turnpoint"));
    canvas.DrawText(rc.left + line_height + padding,
                    rc.top + line_height / 2 - name_font.GetHeight() / 2,
                    buffer);
    return;
  }

  const OrderedTaskPoint &tp = ordered_task->GetTaskPoint(DrawListIndex);
  GeoVector leg = tp.GetNominalLegVector();
  bool show_leg_info = leg.distance > fixed(0.01);

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.Select(small_font);

  UPixelScalar leg_info_width = 0;
  if (show_leg_info) {
    // Draw leg distance
    FormatUserDistanceSmart(leg.distance, buffer, true);
    UPixelScalar width = leg_info_width = canvas.CalcTextWidth(buffer);
    canvas.DrawText(rc.right - padding - width,
                    rc.top + padding +
                    (name_font.GetHeight() - small_font.GetHeight()) / 2,
                    buffer);

    // Draw leg bearing
    FormatBearing(buffer, ARRAY_SIZE(buffer), leg.bearing);
    width = canvas.CalcTextWidth(buffer);
    canvas.DrawText(rc.right - padding - width, top2, buffer);

    if (width > leg_info_width)
      leg_info_width = width;

    leg_info_width += padding;
  }

  // Draw details line
  PixelScalar left = rc.left + line_height + padding;
  OrderedTaskPointRadiusLabel(tp.GetObservationZone(), buffer);
  if (!StringIsEmpty(buffer))
    canvas.DrawClippedText(left, top2, rc.right - leg_info_width - left,
                           buffer);

  // Draw turnpoint name
  canvas.Select(name_font);
  OrderedTaskPointLabel(tp.GetType(), tp.GetWaypoint().name.c_str(),
                        DrawListIndex, buffer);
  canvas.DrawClippedText(left, rc.top + padding,
                         rc.right - leg_info_width - left, buffer);

  // Draw icon
  const RasterPoint pt(rc.left + line_height / 2,
                       rc.top + line_height / 2);

  PixelScalar radius = std::min(PixelScalar(line_height / 2
                                            - Layout::FastScale(4)),
                                Layout::FastScale(10));

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
TaskEditPanel::CanActivateItem(unsigned index) const
{
  return true;
}

void
TaskEditPanel::EditTaskPoint(unsigned ItemIndex)
{
  if (ItemIndex < ordered_task->TaskSize()) {
    if (dlgTaskPointShowModal(&ordered_task, ItemIndex)) {
      *task_modified = true;
      ordered_task->ClearName();
      ordered_task->UpdateGeometry();
      RefreshView();
    }
  } else if (!ordered_task->IsFull()) {

    OrderedTaskPoint* point = nullptr;
    AbstractTaskFactory &factory = ordered_task->GetFactory();
    const Waypoint* way_point =
      ShowWaypointListDialog(ordered_task->TaskSize() > 0
                             ? ordered_task->GetPoint(ordered_task->TaskSize() - 1).GetLocation()
                             : CommonInterface::Basic().location,
                        ordered_task, ItemIndex);
    if (!way_point)
      return;

    if (ItemIndex == 0) {
      point = (OrderedTaskPoint*)factory.CreateStart(*way_point);
    } else {
      point = (OrderedTaskPoint*)factory.CreateIntermediate(*way_point);
     }
    if (point == nullptr)
      return;

    if (factory.Append(*point, true)) {
      *task_modified = true;
      ordered_task->ClearName();
      ordered_task->UpdateGeometry();
      RefreshView();
    }

    delete point;
  }
}

void
TaskEditPanel::OnActivateItem(unsigned index)
{
  EditTaskPoint(index);
}

void
TaskEditPanel::OnCursorMoved(unsigned index)
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

bool
TaskEditPanel::KeyPress(unsigned key_code)
{
  switch (key_code){
  case KEY_ESCAPE:
    if (IsAltair() && GetList().HasFocus()){
       dialog.FocusFirstControl();
      return true;
    }
    return false;

  case '6': /* F5 */
    if (IsAltair()) {
      MoveUp();
      return true;
    } else
      return false;

  case '7': /* F6 */
    if (IsAltair()) {
      MoveDown();
      return true;
    } else
      return false;

  default:
    return false;
  }
}

void
TaskEditPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  UPixelScalar line_height = look.list.font_bold->GetHeight()
    + Layout::Scale(6) + look.small_font->GetHeight();
  CreateList(parent, look, rc, line_height);

  ordered_task = *ordered_task_pointer;;
}

void
TaskEditPanel::ReClick()
{
  dialog.TaskViewClicked();
}

void
TaskEditPanel::Show(const PixelRect &rc)
{
  dialog.ShowTaskView();

  if (ordered_task != *ordered_task_pointer) {
    ordered_task = *ordered_task_pointer;
    GetList().SetCursorIndex(0);
  }

  RefreshView();

  ListWidget::Show(rc);
}

void
TaskEditPanel::Hide()
{
  dialog.ResetTaskView();

  ListWidget::Hide();
}

Widget *
CreateTaskEditPanel(TaskManagerDialog &dialog,
                    const TaskLook &task_look,
                    const AirspaceLook &airspace_look,
                    OrderedTask **active_task, bool *task_modified)
{
  TaskEditButtons *buttons = new TaskEditButtons();
  TextWidget *summary = new TextWidget();
  TaskEditPanel *widget = new TaskEditPanel(dialog, task_look, airspace_look,
                                            active_task, task_modified,
                                            *summary, *buttons);
  buttons->SetListener(*widget);
  TwoWidgets *tw1 = new TwoWidgets(widget, summary);
  widget->SetTwoWidgets(*tw1);
  TwoWidgets *tw2 = new TwoWidgets(tw1, buttons);
  return tw2;
}
