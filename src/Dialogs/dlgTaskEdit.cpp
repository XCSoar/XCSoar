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
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Components.hpp"
#include "Gauge/TaskView.hpp"
#include "Logger/Logger.hpp"
#include "Task/ProtectedTaskManager.hpp"

#include <assert.h>
#include <stdio.h>

static SingleWindow *parent_window;
static WndForm *wf = NULL;
static WndOwnerDrawFrame* wTaskView = NULL;
static RECT TaskViewRect;
static RECT TaskPointsRect;
static RECT TaskSummaryRect;
static bool fullscreen;
static WndListFrame* wTaskPoints = NULL;
static OrderedTask* ordered_task = NULL;
static bool task_modified = false;
static bool show_more = false;

static void ToggleMore();
/**
 * Validates task and prompts if change or error
 * Commits task if no error
 * @return True if task manager should close
 *          False if window should remain open
 */
static bool
CommitTaskChanges()
{
  if (!task_modified)
    return true;

  if (!ordered_task->task_size() || ordered_task->check_task()) {
    MessageBoxX(_("Active task modified"),
                _T("Task Manager"), MB_OK);

    ordered_task->check_duplicate_waypoints(way_points);
    protected_task_manager->task_commit(*ordered_task);
    protected_task_manager->task_save_default();

    task_modified = false;
    return true;
  } else {
    MessageBoxX(getTaskValidationErrors(
        ordered_task->get_factory().getValidationErrors()),
        _("Validation Errors"), MB_ICONEXCLAMATION);
    if (MessageBoxX(_("Task not valid. Changes will be lost.\nContinue?"),
                         _("Task Manager"),
                         MB_YESNO | MB_ICONQUESTION) == IDYES) {
      return true;
    }
  }
  return false;
}

/**
 * Todo add OnCancel
 */
static void
OnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  if (CommitTaskChanges())
    wf->SetModalResult(mrOK);
}

static void
RefreshView()
{
  if (!ordered_task->is_max_size())
    wTaskPoints->SetLength(ordered_task->task_size()+1);
  else
    wTaskPoints->SetLength(ordered_task->task_size());

  wTaskView->invalidate();
  wTaskPoints->invalidate();

  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  if (wSummary) {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text, !Layout::landscape);
    wSummary->SetCaption(text);
  }
}

static void
OnPropertiesClicked(WndButton &Sender)
{
  (void)Sender;
  task_modified |= dlgTaskPropertiesShowModal(*parent_window, &ordered_task);
  RefreshView();
}

static void
OnNewClicked(WndButton &Sender)
{
  (void)Sender;

  OrderedTask::Factory_t new_type = OrderedTask::FACTORY_FAI_GENERAL;
  if (dlgTaskTypeShowModal(*parent_window, &ordered_task, new_type)) {
    ordered_task->clear();
    ordered_task->set_factory(new_type);
    task_modified = true;
    if (show_more)
      ToggleMore();
  }

  RefreshView();
}

static void
ToggleMore()
{
  RECT taskviewrect = TaskViewRect;
  RECT tasksummaryrect = TaskSummaryRect;
  RECT taskpointsrect = TaskPointsRect;

  WndButton* wbMore = ((WndButton*)wf->FindByName(_T("cmdMore")));
  WndFrame* wMore = ((WndFrame*)wf->FindByName(_T("frmMore")));
  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));


  if (wbMore && wMore && wSummary) {
    const unsigned int h = wMore->get_height();
    show_more = !show_more;
    if (show_more) {
      wbMore->set_text(_T("... Less"));
      wMore->show();
      if(Layout::landscape) {
        taskpointsrect.bottom = taskpointsrect.bottom - h;
      }
      else {
        taskviewrect.top = taskviewrect.top + h;
        tasksummaryrect.top = tasksummaryrect.top + h;
      }
    }
    else {
      wbMore->set_text(_T("More ..."));
      wMore->hide();
    }
    if(Layout::landscape) {
      wTaskPoints->move(taskpointsrect.left, taskpointsrect.top,
          taskpointsrect.right - taskpointsrect.left,
          taskpointsrect.bottom - taskpointsrect.top);
    }
    else {
      wTaskView->move(taskviewrect.left, taskviewrect.top,
          taskviewrect.right - taskviewrect.left,
          taskviewrect.bottom - taskviewrect.top);

      wSummary->move(tasksummaryrect.left, tasksummaryrect.top,
          tasksummaryrect.right - tasksummaryrect.left,
          tasksummaryrect.bottom - tasksummaryrect.top);
    }
  }
}

static void
OnMoreClicked(WndButton &Sender)
{
  (void)Sender;
  ToggleMore();
  RefreshView();
}

static void
OnListClicked(WndButton &Sender)
{
  (void)Sender;
  task_modified |= dlgTaskListShowModal(*parent_window, &ordered_task);
  RefreshView();
}

static void
OnDeclareClicked(WndButton &Sender)
{
  (void)Sender;
  logger.LoggerDeviceDeclare(*ordered_task);
}

static void
OnSaveClicked(WndButton &Sender)
{
  (void)Sender;
  if (!ordered_task->check_task()) {
    MessageBoxX(getTaskValidationErrors(
           ordered_task->get_factory().getValidationErrors()),
           _("Validation Errors"),
           MB_ICONEXCLAMATION);
    MessageBoxX (_("Task invalid.  Not saved."),
                 _T("Task Edit"), MB_OK);
    return;
  }

  if (OrderedTaskSave(*ordered_task, true)) {
    MessageBoxX (_("Task saved"),
                 _T("Task Edit"), MB_OK);
  }
}

/**
 * Todo: combine OnClear with OnNew.  in 6.1 they will be the same.
 */
static void
OnClearClicked(WndButton &Sender)
{
  (void)Sender;

  if ((ordered_task->task_size() < 2) ||
      (MessageBoxX(_("Clear task?"),
                   _("Task edit"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    ordered_task->clear();
    ordered_task->set_factory(ordered_task->get_factory_type());
    task_modified = true;
    OnMoreClicked(Sender);
    RefreshView();
  }
}

static void
OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  PaintTask(canvas, Sender->get_client_rect(), *ordered_task,
            XCSoarInterface::Basic().Location,
            XCSoarInterface::SettingsMap(), terrain);
}

static void
OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
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

static void
OnTaskListEnter(unsigned ItemIndex)
{
  if (ItemIndex < ordered_task->task_size()) {
    if (dlgTaskPointShowModal(*parent_window, &ordered_task, ItemIndex)) {
      task_modified = true;
      RefreshView();
    }
  } else if (!ordered_task->is_max_size()) {
    if (dlgTaskPointNew(*parent_window, &ordered_task, ItemIndex)) {
      task_modified = true;
      RefreshView();
    }
  }
}

static void
OnMoveUpClicked(WndButton &Sender)
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index == 0)
    return;

  if (!ordered_task->get_factory().swap(index - 1, true))
    return;

  wTaskPoints->SetCursorIndex(index - 1);
  task_modified = true;
  RefreshView();
}

static void
OnMoveDownClicked(WndButton &Sender)
{
  if (!wTaskPoints)
    return;

  unsigned index = wTaskPoints->GetCursorIndex();
  if (index >= ordered_task->task_size())
    return;

  if (!ordered_task->get_factory().swap(index, true))
    return;

  wTaskPoints->SetCursorIndex(index + 1);
  task_modified = true;
  RefreshView();
}

static bool
OnTaskViewClick(WndOwnerDrawFrame *Sender, int x, int y)
{
  if (!fullscreen) {
    if(show_more)
      ToggleMore();
    wTaskView->move(0, 0, wf->GetClientAreaWindow().get_width(),
                    wf->GetClientAreaWindow().get_height());
    fullscreen = true;
  } else {
    wTaskView->move(TaskViewRect.left, TaskViewRect.top,
                    TaskViewRect.right - TaskViewRect.left,
                    TaskViewRect.bottom - TaskViewRect.top);
    fullscreen = false;
  }
  return true;
}

static bool
OnKeyDown(WndForm &Sender, unsigned key_code)
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

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnPropertiesClicked),
  DeclareCallBackEntry(OnNewClicked),
  DeclareCallBackEntry(OnClearClicked),
  DeclareCallBackEntry(OnMoveUpClicked),
  DeclareCallBackEntry(OnMoveDownClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(OnMoreClicked),
  DeclareCallBackEntry(OnListClicked),
  DeclareCallBackEntry(OnDeclareClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgTaskEditShowModal(SingleWindow &parent)
{
  if (protected_task_manager == NULL)
    return;

  parent_window = &parent;
  task_modified = false;
  show_more = false;

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable,
                        parent, _T("IDR_XML_TASKEDIT_L"));
  else
    wf = LoadDialog(CallBackTable,
                        parent, _T("IDR_XML_TASKEDIT"));

  assert(wf != NULL);
  if (!wf)
    return;

  ordered_task = protected_task_manager->task_clone();
  task_modified = false;

  wTaskPoints = (WndListFrame*)wf->FindByName(_T("frmTaskPoints"));
  assert(wTaskPoints != NULL);
  TaskPointsRect = wTaskPoints->get_position();

  wTaskView = (WndOwnerDrawFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);

  TaskViewRect = wTaskView->get_position();
  wTaskView->SetOnMouseDownNotify(OnTaskViewClick);
  fullscreen = false;

  WndFrame* wTaskSummary = (WndFrame*)wf->FindByName(_T("frmSummary"));
  assert(wTaskSummary != NULL);
  TaskSummaryRect = wTaskSummary->get_position();

  wTaskPoints->SetActivateCallback(OnTaskListEnter);
  wTaskPoints->SetPaintItemCallback(OnTaskPaintListItem);

  wf->SetKeyDownNotify(OnKeyDown);

  RefreshView();

  wf->ShowModal();
  delete wf;
  delete ordered_task;
}
