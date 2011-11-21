/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Dialogs/Task.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Fonts.hpp"
#include "Interface.hpp"
#include "Screen/SingleWindow.hpp"
#include "Form/Frame.hpp"
#include "Form/List.hpp"
#include "Form/Draw.hpp"
#include "Form/TabBar.hpp"
#include "Form/Util.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h>

class WndButton;

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskEditPanel *instance;

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

  if (!ordered_task->is_max_size())
    wTaskPoints->SetLength(ordered_task->TaskSize()+1);
  else
    wTaskPoints->SetLength(ordered_task->TaskSize());

  wTaskView->invalidate();
  wTaskPoints->invalidate();

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
      (MessageBoxX(_("Clear all points?"), _("Task edit"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    while (ordered_task->TaskSize())
      ordered_task->Remove(0);

    *task_modified = true;
    RefreshView();
  }
}

static void
OnClearAllClicked(gcc_unused WndButton &Sender)
{
  instance->OnClearAllClicked();
}

void
TaskEditPanel::OnTaskPaintListItem(Canvas &canvas, const PixelRect rc,
                                   unsigned DrawListIndex)
{
  assert(DrawListIndex <= ordered_task->TaskSize());

  const PixelScalar line_height = rc.bottom - rc.top;

  TCHAR buffer[120];

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  // Draw "Add turnpoint" label
  if (DrawListIndex == ordered_task->TaskSize()) {
    canvas.select(name_font);
    _stprintf(buffer, _T("  (%s)"), _("Add Turnpoint"));
    canvas.text(rc.left + line_height + Layout::FastScale(2),
                rc.top + line_height / 2 - name_font.GetHeight() / 2, buffer);
    return;
  }

  const OrderedTaskPoint &tp = *ordered_task->GetTaskPoint(DrawListIndex);
  GeoVector leg = tp.leg_vector_nominal();
  bool show_leg_info = leg.distance > fixed(0.01);

  // Draw icon
  RasterPoint pt = { PixelScalar(rc.left + line_height / 2),
                     PixelScalar(rc.top + line_height / 2) };
  WaypointIconRenderer wir(CommonInterface::SettingsMap().waypoint,
                           CommonInterface::main_window.GetLook().map.waypoint,
                           canvas);

  wir.Draw(tp.GetWaypoint(), pt, WaypointIconRenderer::Unreachable, true);

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.select(small_font);

  UPixelScalar leg_info_width = 0;
  if (show_leg_info) {
    // Draw leg distance
    Units::FormatUserDistance(leg.distance, buffer, 120, true);
    UPixelScalar width = leg_info_width = canvas.text_width(buffer);
    canvas.text(rc.right - Layout::FastScale(2) - width,
                rc.top + Layout::FastScale(2) +
                (name_font.GetHeight() - small_font.GetHeight()) / 2, buffer);

    // Draw leg bearing
    _stprintf(buffer, _T("%.0f" DEG " T"), (double)leg.bearing.Degrees());
    width = canvas.text_width(buffer);
    canvas.text(rc.right - Layout::FastScale(2) - width, top2, buffer);

    if (width > leg_info_width)
      leg_info_width = width;

    leg_info_width += Layout::FastScale(2);
  }

  // Draw details line
  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  OrderedTaskPointRadiusLabel(*tp.get_oz(), buffer);
  if (!string_is_empty(buffer))
    canvas.text_clipped(left, top2, rc.right - leg_info_width - left, buffer);

  // Draw turnpoint name
  canvas.select(name_font);
  OrderedTaskPointLabel(tp.GetType(), tp.GetWaypoint().name.c_str(),
                        DrawListIndex, buffer);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2),
                      rc.right - leg_info_width - left, buffer);
}

static void
OnTaskPaintListItem(Canvas &canvas, const PixelRect rc, unsigned DrawListIndex)
{
  instance->OnTaskPaintListItem(canvas, rc, DrawListIndex);
}

void
TaskEditPanel::OnEditTurnpointClicked()
{
  OnTaskListEnter(wTaskPoints->GetCursorIndex());
}

static void
OnEditTurnpointClicked(gcc_unused WndButton &Sender)
{
  instance->OnEditTurnpointClicked();
}

void
TaskEditPanel::OnTaskListEnter(unsigned ItemIndex)
{
  if (ItemIndex < ordered_task->TaskSize()) {
    if (dlgTaskPointShowModal(wf.GetMainWindow(), &ordered_task, ItemIndex)) {
      *task_modified = true;
      RefreshView();
    }
  } else if (!ordered_task->is_max_size()) {

    OrderedTaskPoint* point = NULL;
    AbstractTaskFactory &factory = ordered_task->GetFactory();
    const Waypoint* way_point =
      dlgWaypointSelect(wf.GetMainWindow(),
                        ordered_task->TaskSize() > 0 ?
                          ordered_task->get_tp(ordered_task->
                              TaskSize() - 1)->GetLocation() :
                          XCSoarInterface::Basic().location,
                        ordered_task, ItemIndex);
    if (!way_point)
      return;

    if (ItemIndex == 0) {
      point = (OrderedTaskPoint*)factory.createStart(*way_point);
    } else {
      point = (OrderedTaskPoint*)factory.createIntermediate(*way_point);
     }
    if (point == NULL)
      return;

    if (factory.append(*point, true))
      *task_modified = true;

    delete point;

    RefreshView();
  }
}

static void
OnTaskListEnter(unsigned ItemIndex)
{
  instance->OnTaskListEnter(ItemIndex);
}

static void
OnTaskCursorCallback(gcc_unused unsigned i)
{
  instance->UpdateButtons();
}

void
TaskEditPanel::OnMakeFinish()
{
  ordered_task->GetFactory().CheckAddFinish();
  RefreshView();
}

static void
OnMakeFinish(gcc_unused WndButton &Sender)
{
  instance->OnMakeFinish();
}

void
TaskEditPanel::MoveUp()
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index == 0)
    return;

  if (!ordered_task->GetFactory().swap(index - 1, true))
    return;

  wTaskPoints->SetCursorIndex(index - 1);
  *task_modified = true;

  RefreshView();
}

static void
OnMoveUpClicked(gcc_unused WndButton &Sender)
{
  instance->MoveUp();
}

void
TaskEditPanel::MoveDown()
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index >= ordered_task->TaskSize())
    return;

  if (!ordered_task->GetFactory().swap(index, true))
    return;

  wTaskPoints->SetCursorIndex(index + 1);
  *task_modified = true;

  RefreshView();
}

static void
OnMoveDownClicked(gcc_unused WndButton &Sender)
{
  instance->MoveDown();
}

bool
TaskEditPanel::OnKeyDown(unsigned key_code)
{
  switch (key_code){
  case VK_ESCAPE:
    if (is_altair() && wTaskPoints->has_focus()){
       wf.focus_first_control();
      return true;
    }
    return false;

  case '6': /* F5 */
    if (is_altair()) {
      MoveUp();
      return true;
    } else
      return false;

  case '7': /* F6 */
    if (is_altair()) {
      MoveDown();
      return true;
    } else
      return false;

  default:
    return false;
  }
}

static bool
OnKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  return instance->OnKeyDown(key_code);
}

static gcc_constexpr_data CallBackTableEntry task_edit_callbacks[] = {
  DeclareCallBackEntry(dlgTaskManager::OnTaskPaint),

  DeclareCallBackEntry(OnMakeFinish),
  DeclareCallBackEntry(OnMoveUpClicked),
  DeclareCallBackEntry(OnMoveDownClicked),
  DeclareCallBackEntry(OnEditTurnpointClicked),
  DeclareCallBackEntry(OnClearAllClicked),

  DeclareCallBackEntry(NULL)
};

void
TaskEditPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  ordered_task = *ordered_task_pointer;;

  LoadWindow(task_edit_callbacks, parent,
             Layout::landscape
             ? _T("IDR_XML_TASKEDIT_L") : _T("IDR_XML_TASKEDIT"));

  instance = this;

  wTaskPoints = (WndListFrame*)form.FindByName(_T("frmTaskPoints"));
  assert(wTaskPoints != NULL);

  wTaskView = (WndOwnerDrawFrame*)form.FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);
  wTaskView->SetOnMouseDownNotify(dlgTaskManager::OnTaskViewClick);

  wSummary = (WndFrame *)form.FindByName(_T("frmSummary"));
  assert(wSummary);

  UPixelScalar line_height = Fonts::MapBold.GetHeight() + Layout::Scale(6) +
    Fonts::MapLabel.GetHeight();
  wTaskPoints->SetItemHeight(line_height);
  wTaskPoints->SetActivateCallback(::OnTaskListEnter);
  wTaskPoints->SetPaintItemCallback(::OnTaskPaintListItem);
  wTaskPoints->SetCursorCallback(::OnTaskCursorCallback);
}

void
TaskEditPanel::ReClick()
{
  dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
}

void
TaskEditPanel::Show(const PixelRect &rc)
{
  if (ordered_task != *ordered_task_pointer) {
    ordered_task = *ordered_task_pointer;
    wTaskPoints->SetCursorIndex(0);
  }
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();

  wf.SetKeyDownNotify(::OnKeyDown);

  XMLWidget::Show(rc);
}

void
TaskEditPanel::Hide()
{
  wf.SetKeyDownNotify(NULL);

  XMLWidget::Hide();
}
