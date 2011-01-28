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

#include "Dialogs/Internal.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Dialogs/dlgTaskManager.hpp"

#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Components.hpp"
#include "Gauge/TaskView.hpp"
#include "Screen/SingleWindow.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm* wf = NULL;
static WndOwnerDrawFrame* wTaskView = NULL;
static WndFrame* wSummary = NULL;
static RECT TaskViewRect;
static RECT TaskPointsRect;
static RECT TaskSummaryRect;
static bool fullscreen;
static WndListFrame* wTaskPoints = NULL;
static OrderedTask* ordered_task = NULL;
static OrderedTask** ordered_task_pointer = NULL;
static bool* task_modified = NULL;

void
dlgTaskEdit::RefreshView()
{
  if (!ordered_task->is_max_size())
    wTaskPoints->SetLength(ordered_task->task_size()+1);
  else
    wTaskPoints->SetLength(ordered_task->task_size());

  wTaskView->invalidate();
  wTaskPoints->invalidate();

  if (wSummary) {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text, !Layout::landscape);
    wSummary->SetCaption(text);
  }
}

void
dlgTaskEdit::OnNewClicked(WndButton &Sender)
{
  (void)Sender;

  OrderedTask::Factory_t new_type = OrderedTask::FACTORY_FAI_GENERAL;
  if (dlgTaskTypeShowModal(*parent_window, &ordered_task, new_type)) {
    ordered_task->clear();
    ordered_task->set_factory(new_type);
    *task_modified = true;
  }

  RefreshView();
}

// ToDo: make this generic factory type and merge with OnNew
void
dlgTaskEdit::OnClearClicked(WndButton &Sender)
{
  (void)Sender;

  if ((ordered_task->task_size() < 2) ||
      (MessageBoxX(_("Clear task?"), _("Task edit"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    ordered_task->clear();
    ordered_task->set_factory(ordered_task->get_factory_type());
    *task_modified = true;
    RefreshView();
  }
}

void
dlgTaskEdit::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  PaintTask(canvas, Sender->get_client_rect(), *ordered_task,
            XCSoarInterface::Basic().Location,
            XCSoarInterface::SettingsMap(), terrain);
}

void
dlgTaskEdit::OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  assert(DrawListIndex <= ordered_task->task_size());

  TCHAR sTmp[120];

  if (DrawListIndex == ordered_task->task_size()) {
    if (!ordered_task->is_max_size()) {
      _stprintf(sTmp, _T("  (%s)"), _("add waypoint"));
      canvas.text(rc.left + Layout::FastScale(2),
                  rc.top + Layout::FastScale(2), sTmp);
    }
  } else {
    TCHAR sRad[10];
    TCHAR sDist[10];
    fixed fDist = fixed_zero;
    int w0, wRad, wDist, x;

    w0 = rc.right - rc.left - Layout::FastScale(4);
    wRad = canvas.text_width(_T("XXXkm"));
    wDist = canvas.text_width(_T("00000km"));
    x = w0 - wRad - wDist;

    OrderedTaskPointLabel(ordered_task, DrawListIndex, sTmp, sRad);

    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + Layout::FastScale(2),
                        x - Layout::FastScale(5), sTmp);

    if (sRad[0] != _T('\0')) {
      x = w0 - wDist - canvas.text_width(sRad);
      canvas.text(rc.left + x, rc.top + Layout::FastScale(2), sRad);
    }

    if (DrawListIndex < ordered_task->task_size()) {
      fDist = ordered_task->getTaskPoint(DrawListIndex)->leg_distance_nominal();

      if (fDist > fixed(0.01)) {
        _stprintf(sDist,_T("%.1f%s"), (double)Units::ToUserDistance(fDist),Units::GetUnitName(Units::DistanceUnit));
        x = w0 - canvas.text_width(sDist);
        canvas.text(rc.left + x, rc.top + Layout::FastScale(2), sDist);
      }
    }
  }
}

void
dlgTaskEdit::OnEditTurnpointClicked(WndButton &Sender)
{
  OnTaskListEnter(wTaskPoints->GetCursorIndex());
}

void
dlgTaskEdit::OnTaskListEnter(unsigned ItemIndex)
{
  if (ItemIndex < ordered_task->task_size()) {
    if (dlgTaskPointShowModal(*parent_window, &ordered_task, ItemIndex)) {
      *task_modified = true;
      RefreshView();
    }
  } else if (!ordered_task->is_max_size()) {
    if (dlgTaskPointNew(*parent_window, &ordered_task, ItemIndex)) {
      *task_modified = true;
      RefreshView();
    }
  }
}

void
dlgTaskEdit::OnMoveUpClicked(WndButton &Sender)
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
  RefreshView();
}

void
dlgTaskEdit::OnMoveDownClicked(WndButton &Sender)
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
  RefreshView();
}

bool
dlgTaskEdit::OnTaskViewClick(WndOwnerDrawFrame *Sender, int x, int y)
{
  if (!fullscreen) {
    wTaskView->move(0, 0, wf->GetClientAreaWindow().get_width(),
                    wf->GetClientAreaWindow().get_height());
    fullscreen = true;
    wTaskView->show_on_top();
  } else {
    wTaskView->move(TaskViewRect.left, TaskViewRect.top,
                    TaskViewRect.right - TaskViewRect.left,
                    TaskViewRect.bottom - TaskViewRect.top);
    fullscreen = false;
  }
  wTaskView->invalidate();
  return true;
}

bool
dlgTaskEdit::OnKeyDown(WndForm &Sender, unsigned key_code)
{
  switch (key_code){
  case VK_ESCAPE:
    if (is_altair() && wTaskPoints->has_focus()){
       wf->focus_first_control();
      return true;
    }
    return false;
  default:
    return false;
  }
}

bool
dlgTaskEdit::OnTabPreShow()
{
  if (ordered_task != *ordered_task_pointer) {
    ordered_task = *ordered_task_pointer;
    wTaskPoints->SetCursorIndex(0);
  }
  RefreshView();
  return true;
}

Window*
dlgTaskEdit::Load(SingleWindow &parent, TabBarControl* wTabBar, WndForm* _wf,
                  OrderedTask** task, bool* _task_modified)
{
  assert(wTabBar);
  assert(_wf);
  wf = _wf;

  assert (task);
  ordered_task_pointer = task;

  assert(*task);
  ordered_task = *ordered_task_pointer;;

  assert(_task_modified);
  task_modified = _task_modified;

  parent_window = &parent;

  Window *wTps =
      LoadWindow(dlgTaskManager::CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                 _T("IDR_XML_TASKEDIT_L") : _T("IDR_XML_TASKEDIT"));
  assert(wTps);

  wTaskPoints = (WndListFrame*)wf->FindByName(_T("frmTaskPoints"));
  assert(wTaskPoints != NULL);
  TaskPointsRect = wTaskPoints->get_position();

  wTaskView = (WndOwnerDrawFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);

  TaskViewRect = wTaskView->get_position();
  wTaskView->SetOnMouseDownNotify(OnTaskViewClick);
  fullscreen = false;

  wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  assert(wSummary);
  TaskSummaryRect = wSummary->get_position();

  wTaskPoints->SetActivateCallback(OnTaskListEnter);
  wTaskPoints->SetPaintItemCallback(OnTaskPaintListItem);

  //Todo: fix onkey down.  release on hiding?
  wf->SetKeyDownNotify(OnKeyDown);

  RefreshView();

  return wTps;
}
