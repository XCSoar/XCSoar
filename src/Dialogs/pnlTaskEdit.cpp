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

#include "Dialogs/Task.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Dialogs/dlgTaskManager.hpp"
#include "Dialogs/Waypoint.hpp"

#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Fonts.hpp"
#include "Interface.hpp"
#include "Screen/SingleWindow.hpp"
#include "Form/TabBar.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Waypoint/WaypointIconRenderer.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"

#include <assert.h>
#include <stdio.h>

static WndForm* wf = NULL;
static TabBarControl* wTabBar = NULL;
static WndOwnerDrawFrame* wTaskView = NULL;
static WndFrame* wSummary = NULL;
static PixelRect TaskSummaryRect;
static WndListFrame* wTaskPoints = NULL;
static OrderedTask* ordered_task = NULL;
static OrderedTask** ordered_task_pointer = NULL;
static bool* task_modified = NULL;

static void
UpdateButtons()
{
  const unsigned index = wTaskPoints->GetCursorIndex();
  // Todo check if point is already finish
  ShowFormControl(*wf, _T("cmdMakeFinish"),
                  index > 0 &&
                  (index == ordered_task->task_size() - 1) &&
                  !ordered_task->has_finish());

  ShowFormControl(*wf, _T("cmdDown"),
                  (int)index < ((int)(ordered_task->task_size()) - 1));

  ShowFormControl(*wf, _T("cmdUp"),
                  index > 0 && index < ordered_task->task_size());

  ShowFormControl(*wf, _T("cmdEditTurnpoint"),
                  index < ordered_task->task_size());
}

void
pnlTaskEdit::RefreshView()
{
  UpdateButtons();

  if (!ordered_task->is_max_size())
    wTaskPoints->SetLength(ordered_task->task_size()+1);
  else
    wTaskPoints->SetLength(ordered_task->task_size());

  wTaskView->invalidate();
  wTaskPoints->invalidate();

  if (wSummary) {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text, false);
    wSummary->SetCaption(text);
  }
}

void
pnlTaskEdit::OnClearAllClicked(gcc_unused WndButton &Sender)
{
  if ((ordered_task->task_size() < 2) ||
      (MessageBoxX(_("Clear all points?"), _("Task edit"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    while (ordered_task->task_size())
      ordered_task->remove(0);

    *task_modified = true;
    RefreshView();
  }
}

void
pnlTaskEdit::OnTaskPaintListItem(Canvas &canvas, const PixelRect rc,
                                 unsigned DrawListIndex)
{
  assert(DrawListIndex <= ordered_task->task_size());

  const unsigned line_height = rc.bottom - rc.top;

  TCHAR sTmp[120];

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;
  canvas.select(name_font);

  // Draw "Add turnpoint" label
  if (DrawListIndex == ordered_task->task_size()) {
    _stprintf(sTmp, _T("  (%s)"), _("Add Turnpoint"));
    canvas.text(rc.left + line_height + Layout::FastScale(2),
                rc.top + line_height / 2 - name_font.get_height() / 2, sTmp);
    return;
  }

  const OrderedTaskPoint &tp = *ordered_task->getTaskPoint(DrawListIndex);
  GeoVector leg = tp.leg_vector_nominal();
  bool show_leg_information = leg.Distance > fixed(0.01);

  // Draw icon
  RasterPoint pt = { rc.left + line_height / 2,
                     rc.top + line_height / 2};
  WaypointIconRenderer wir(CommonInterface::SettingsMap().waypoint,
                           CommonInterface::main_window.look->waypoint,
                           canvas);
  wir.Draw(tp.get_waypoint(), pt,
           WaypointIconRenderer::Unreachable, true);

  // Draw leg distance
  if (show_leg_information) {
    TCHAR dist[20];
    Units::FormatUserDistance(leg.Distance, dist, 20, true);
    canvas.text_clipped(rc.right - Layout::FastScale(2) - canvas.text_width(dist),
                        rc.top + Layout::FastScale(2), rc, dist);
  }

  // Draw turnpoint name
  OrderedTaskPointLabel(tp, DrawListIndex, sTmp);
  canvas.text_clipped(rc.left + line_height + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc, sTmp);

  // Draw details line
  canvas.select(small_font);

  OrderedTaskPointRadiusLabel(*tp.get_oz(), sTmp);
  if (!string_is_empty(sTmp))
    canvas.text_clipped(rc.left + line_height + Layout::FastScale(2),
                        rc.top + name_font.get_height() + Layout::FastScale(4),
                        rc, sTmp);

  if (show_leg_information) {
    _stprintf(sTmp, _T("%s: %.0f" DEG "T"),
              _("Bearing"), (double)leg.Bearing.value_degrees());
    canvas.text_clipped(rc.right - Layout::FastScale(2) - canvas.text_width(sTmp),
                        rc.top + name_font.get_height() + Layout::FastScale(4),
                        rc, sTmp);
  }
}

void
pnlTaskEdit::OnEditTurnpointClicked(gcc_unused WndButton &Sender)
{
  OnTaskListEnter(wTaskPoints->GetCursorIndex());
}

void
pnlTaskEdit::OnTaskListEnter(unsigned ItemIndex)
{
  if (ItemIndex < ordered_task->task_size()) {
    if (dlgTaskPointShowModal(wf->GetMainWindow(), &ordered_task, ItemIndex)) {
      *task_modified = true;
      RefreshView();
    }
  } else if (!ordered_task->is_max_size()) {

    OrderedTaskPoint* point = NULL;
    AbstractTaskFactory &factory = ordered_task->get_factory();
    const Waypoint* way_point =
      dlgWaypointSelect(wf->GetMainWindow(),
                        ordered_task->task_size() > 0 ?
                          ordered_task->get_tp(ordered_task->
                              task_size() - 1)->get_location() :
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

void
pnlTaskEdit::OnTaskCursorCallback(gcc_unused unsigned i)
{
  UpdateButtons();
}
void
pnlTaskEdit::OnMakeFinish(gcc_unused WndButton &Sender)
{
  ordered_task->get_factory().CheckAddFinish();
  RefreshView();
}

static void
MoveUp()
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index == 0)
    return;

  if (!ordered_task->get_factory().swap(index - 1, true))
    return;

  wTaskPoints->SetCursorIndex(index - 1);
  *task_modified = true;
  pnlTaskEdit::RefreshView();
}

void
pnlTaskEdit::OnMoveUpClicked(gcc_unused WndButton &Sender)
{
  MoveUp();
}

static void
MoveDown()
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index >= ordered_task->task_size())
    return;

  if (!ordered_task->get_factory().swap(index, true))
    return;

  wTaskPoints->SetCursorIndex(index + 1);
  *task_modified = true;
  pnlTaskEdit::RefreshView();
}

void
pnlTaskEdit::OnMoveDownClicked(gcc_unused WndButton &Sender)
{
  MoveDown();
}

bool
pnlTaskEdit::OnKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code){
  case VK_ESCAPE:
    if (is_altair() && wTaskPoints->has_focus()){
       wf->focus_first_control();
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

bool
pnlTaskEdit::OnTabPreShow(gcc_unused TabBarControl::EventType EventType)
{
  if (ordered_task != *ordered_task_pointer) {
    ordered_task = *ordered_task_pointer;
    wTaskPoints->SetCursorIndex(0);
  }
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();
  return true;
}

void
pnlTaskEdit::OnTabReClick()
{
  dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
}

Window*
pnlTaskEdit::Load(SingleWindow &parent, TabBarControl* _wTabBar, WndForm* _wf,
                  OrderedTask** task, bool* _task_modified)
{
  assert(_wTabBar);
  wTabBar = _wTabBar;

  assert(_wf);
  wf = _wf;

  assert (task);
  ordered_task_pointer = task;

  assert(*task);
  ordered_task = *ordered_task_pointer;;

  assert(_task_modified);
  task_modified = _task_modified;

  Window *wTps =
      LoadWindow(dlgTaskManager::CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                 _T("IDR_XML_TASKEDIT_L") : _T("IDR_XML_TASKEDIT"));
  assert(wTps);

  wTaskPoints = (WndListFrame*)wf->FindByName(_T("frmTaskPoints"));
  assert(wTaskPoints != NULL);

  wTaskView = (WndOwnerDrawFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);
  wTaskView->SetOnMouseDownNotify(dlgTaskManager::OnTaskViewClick);

  wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  assert(wSummary);
  TaskSummaryRect = wSummary->get_position();

  unsigned line_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                         Fonts::MapLabel.get_height();
  wTaskPoints->SetItemHeight(line_height);
  wTaskPoints->SetActivateCallback(OnTaskListEnter);
  wTaskPoints->SetPaintItemCallback(OnTaskPaintListItem);
  wTaskPoints->SetCursorCallback(OnTaskCursorCallback);

  //Todo: fix onkey down.  release on hiding?
  wf->SetKeyDownNotify(OnKeyDown);

  RefreshView();

  return wTps;
}
