/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Form/Frame.hpp"
#include "Form/List.hpp"
#include "Form/Util.hpp"
#include "Widget/XMLWidget.hpp"
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

#include <assert.h>
#include <stdio.h>

enum Buttons {
  EDIT = 100,
  DOWN,
  UP,
  MUTATE,
  CLEAR_ALL,
};

class TaskEditPanel
  : public XMLWidget, public ActionListener,
    private ListControl::Handler {
  TaskManagerDialog &dialog;

  const TaskLook &task_look;
  const AirspaceLook &airspace_look;

  OrderedTask **ordered_task_pointer, *ordered_task;
  bool *task_modified;

  ListControl *wTaskPoints;
  WndFrame *wSummary;

public:
  TaskEditPanel(TaskManagerDialog &_dialog,
                const TaskLook &_task_look, const AirspaceLook &_airspace_look,
                OrderedTask **_active_task, bool *_task_modified)
    :dialog(_dialog),
     task_look(_task_look), airspace_look(_airspace_look),
     ordered_task_pointer(_active_task), task_modified(_task_modified) {}

  void UpdateButtons();

  void MoveUp();
  void MoveDown();

  void OnClearAllClicked();
  void OnEditTurnpointClicked();
  void EditTaskPoint(unsigned ItemIndex);
  void OnMakeFinish();

  bool OnKeyDown(unsigned key_code);

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void ReClick();
  virtual void Show(const PixelRect &rc);
  virtual void Hide();

protected:
  void RefreshView();

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) gcc_override;

  /* virtual methods from List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx);
  virtual void OnCursorMoved(unsigned index);
  virtual bool CanActivateItem(unsigned index) const;
  virtual void OnActivateItem(unsigned index);
};

void
TaskEditPanel::UpdateButtons()
{
  const unsigned index = wTaskPoints->GetCursorIndex();
  // Todo check if point is already finish
  ShowFormControl(form, _T("cmdMakeFinish"),
                  index > 0 &&
                  (index == ordered_task->TaskSize() - 1) &&
                  !ordered_task->HasFinish());

  ShowFormControl(form, _T("cmdDown"),
                  (int)index < ((int)(ordered_task->TaskSize()) - 1));

  ShowFormControl(form, _T("cmdUp"),
                  index > 0 && index < ordered_task->TaskSize());

  ShowFormControl(form, _T("cmdEditTurnpoint"),
                  index < ordered_task->TaskSize());
}

void
TaskEditPanel::RefreshView()
{
  UpdateButtons();

  dialog.InvalidateTaskView();

  if (!ordered_task->IsFull())
    wTaskPoints->SetLength(ordered_task->TaskSize()+1);
  else
    wTaskPoints->SetLength(ordered_task->TaskSize());
  wTaskPoints->Invalidate();

  if (wSummary) {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text, false);
    wSummary->SetCaption(text);
  }
}

void
TaskEditPanel::OnClearAllClicked()
{
  if ((ordered_task->TaskSize() < 2) ||
      (ShowMessageBox(_("Clear all points?"), _("Task edit"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {

    ordered_task->RemoveAllPoints();

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

  const PixelScalar line_height = rc.bottom - rc.top;

  TCHAR buffer[120];

  const Font &name_font = *dialog.GetLook().list.font;
  const Font &small_font = *dialog.GetLook().small_font;

  // Draw "Add turnpoint" label
  if (DrawListIndex == ordered_task->TaskSize()) {
    canvas.Select(name_font);
    canvas.SetTextColor(COLOR_BLACK);
    _stprintf(buffer, _T("  (%s)"), _("Add Turnpoint"));
    canvas.DrawText(rc.left + line_height + Layout::FastScale(2),
                    rc.top + line_height / 2 - name_font.GetHeight() / 2,
                    buffer);
    return;
  }

  const OrderedTaskPoint &tp = ordered_task->GetTaskPoint(DrawListIndex);
  GeoVector leg = tp.GetNominalLegVector();
  bool show_leg_info = leg.distance > fixed(0.01);

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

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.Select(small_font);
  canvas.SetTextColor(COLOR_BLACK);

  UPixelScalar leg_info_width = 0;
  if (show_leg_info) {
    // Draw leg distance
    FormatUserDistanceSmart(leg.distance, buffer, true);
    UPixelScalar width = leg_info_width = canvas.CalcTextWidth(buffer);
    canvas.DrawText(rc.right - Layout::FastScale(2) - width,
                    rc.top + Layout::FastScale(2) +
                    (name_font.GetHeight() - small_font.GetHeight()) / 2,
                    buffer);

    // Draw leg bearing
    FormatBearing(buffer, ARRAY_SIZE(buffer), leg.bearing);
    width = canvas.CalcTextWidth(buffer);
    canvas.DrawText(rc.right - Layout::FastScale(2) - width, top2, buffer);

    if (width > leg_info_width)
      leg_info_width = width;

    leg_info_width += Layout::FastScale(2);
  }

  // Draw details line
  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  OrderedTaskPointRadiusLabel(tp.GetObservationZone(), buffer);
  if (!StringIsEmpty(buffer))
    canvas.DrawClippedText(left, top2, rc.right - leg_info_width - left,
                           buffer);

  // Draw turnpoint name
  canvas.Select(name_font);
  OrderedTaskPointLabel(tp.GetType(), tp.GetWaypoint().name.c_str(),
                        DrawListIndex, buffer);
  canvas.DrawClippedText(left, rc.top + Layout::FastScale(2),
                         rc.right - leg_info_width - left, buffer);
}

void
TaskEditPanel::OnEditTurnpointClicked()
{
  EditTaskPoint(wTaskPoints->GetCursorIndex());
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
    if (dlgTaskPointShowModal(dialog.GetMainWindow(),
                              &ordered_task, ItemIndex)) {
      *task_modified = true;
      RefreshView();
    }
  } else if (!ordered_task->IsFull()) {

    OrderedTaskPoint* point = NULL;
    AbstractTaskFactory &factory = ordered_task->GetFactory();
    const Waypoint* way_point =
      ShowWaypointListDialog(dialog.GetMainWindow(),
                             ordered_task->TaskSize() > 0
                             ? ordered_task->GetPoint(ordered_task->TaskSize() - 1).GetLocation()
                             : XCSoarInterface::Basic().location,
                        ordered_task, ItemIndex);
    if (!way_point)
      return;

    if (ItemIndex == 0) {
      point = (OrderedTaskPoint*)factory.CreateStart(*way_point);
    } else {
      point = (OrderedTaskPoint*)factory.CreateIntermediate(*way_point);
     }
    if (point == NULL)
      return;

    if (factory.Append(*point, true))
      *task_modified = true;

    delete point;

    RefreshView();
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
  ordered_task->GetFactory().CheckAddFinish();
  RefreshView();
}

void
TaskEditPanel::MoveUp()
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index == 0)
    return;

  if (!ordered_task->GetFactory().Swap(index - 1, true))
    return;

  wTaskPoints->SetCursorIndex(index - 1);
  *task_modified = true;

  RefreshView();
}

void
TaskEditPanel::MoveDown()
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index >= ordered_task->TaskSize())
    return;

  if (!ordered_task->GetFactory().Swap(index, true))
    return;

  wTaskPoints->SetCursorIndex(index + 1);
  *task_modified = true;

  RefreshView();
}

bool
TaskEditPanel::OnKeyDown(unsigned key_code)
{
  switch (key_code){
  case KEY_ESCAPE:
    if (IsAltair() && wTaskPoints->HasFocus()){
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

static void
SetActionListener(SubForm &form, const TCHAR *name,
                  ActionListener *listener, int id)
{
  ((WndButton *)form.FindByName(name))->SetListener(listener, id);
}

void
TaskEditPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  ordered_task = *ordered_task_pointer;;

  LoadWindow(nullptr, parent, rc, _T("IDR_XML_TASKEDIT"));

  SetActionListener(form, _T("cmdEditTurnpoint"), this, EDIT);
  SetActionListener(form, _T("cmdMakeFinish"), this, MUTATE);
  SetActionListener(form, _T("cmdDown"), this, DOWN);
  SetActionListener(form, _T("cmdUp"), this, UP);
  SetActionListener(form, _T("cmdNew"), this, CLEAR_ALL);

  wTaskPoints = (ListControl*)form.FindByName(_T("frmTaskPoints"));
  assert(wTaskPoints != NULL);

  wSummary = (WndFrame *)form.FindByName(_T("frmSummary"));
  assert(wSummary);

  UPixelScalar line_height = dialog.GetLook().list.font->GetHeight()
    + Layout::Scale(6) + dialog.GetLook().small_font->GetHeight();
  wTaskPoints->SetItemHeight(line_height);
  wTaskPoints->SetHandler(this);
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
    wTaskPoints->SetCursorIndex(0);
  }

  RefreshView();

  dialog.SetKeyDownFunction([this](unsigned key_code){
      return this->OnKeyDown(key_code);
    });

  XMLWidget::Show(rc);
}

void
TaskEditPanel::Hide()
{
  dialog.ResetTaskView();
  dialog.ClearKeyDownFunction();

  XMLWidget::Hide();
}

Widget *
CreateTaskEditPanel(TaskManagerDialog &dialog,
                    const TaskLook &task_look,
                    const AirspaceLook &airspace_look,
                    OrderedTask **active_task, bool *task_modified)
{
  return new TaskEditPanel(dialog, task_look, airspace_look,
                           active_task, task_modified);
}
