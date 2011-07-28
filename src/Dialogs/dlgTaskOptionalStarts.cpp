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
#include "Dialogs/Waypoint.hpp"
#include "Screen/Layout.hpp"
#include "LocalPath.hpp"

#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"

#include <assert.h>
#include <stdio.h>

static WndForm *wf = NULL;
static WndListFrame* wOptionalStartPoints = NULL;
static bool task_modified = false;
static OrderedTask* ordered_task = NULL;
static bool RealStartExists = false;
static AbstractTaskFactory::LegalPointVector point_types;

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
RefreshView()
{
  wOptionalStartPoints->SetLength(ordered_task->optional_start_points_size()
      + (RealStartExists ? 2 : 1));

  wOptionalStartPoints->invalidate();
}

static void
OnOptionalStartPaintListItem(Canvas &canvas, const PixelRect rc,
                             unsigned DrawListIndex)
{
  assert(DrawListIndex <= ordered_task->optional_start_points_size()
      +  (RealStartExists ? 2 : 1));
  assert(wOptionalStartPoints->GetLength() ==
      ordered_task->optional_start_points_size() + (RealStartExists ? 2 : 1));

  const unsigned index_optional_starts = DrawListIndex - (RealStartExists ? 1 : 0);

  if (DrawListIndex == wOptionalStartPoints->GetLength() - 1) {
    canvas.text(rc.left + Layout::FastScale(2),
                rc.top + Layout::FastScale(2), _("(Add Alternate Start)"));
  } else {

    TCHAR tmp[MAX_PATH];
    if (DrawListIndex == 0 && RealStartExists) {
      const OrderedTaskPoint *tp = ordered_task->get_tp(0);
      assert(tp);
      _stprintf(tmp,_T("*%s"), tp->get_waypoint().Name.c_str());

    } else {
      const OrderedTaskPoint *tp =
          ordered_task->get_optional_start(index_optional_starts);
      assert(tp);
      _tcscpy(tmp,tp->get_waypoint().Name.c_str());
    }

    canvas.text(rc.left + Layout::FastScale(2),
                rc.top + Layout::FastScale(2), tmp);
  }
}


static void
OnOptionalStartListEnter(unsigned ItemIndex)
{
  assert(ItemIndex <= ordered_task->optional_start_points_size()
      +  (RealStartExists ? 2 : 1));
  assert(wOptionalStartPoints->GetLength() ==
      ordered_task->optional_start_points_size() + (RealStartExists ? 2 : 1));

  if (ItemIndex == 0 && RealStartExists)
    return;

  const unsigned index_optional_starts = ItemIndex - (RealStartExists ? 1 : 0);

  if (index_optional_starts < ordered_task->optional_start_points_size()) {
    const Waypoint* way_point = dlgWaypointSelect(wf->GetMainWindow(),
        ordered_task->task_size() > 0 ? ordered_task->get_tp(0)->get_location()
            : XCSoarInterface::Basic().location);
    if (!way_point)
      return;

    if (ordered_task->relocate_optional_start(index_optional_starts, *way_point))
      task_modified = true;

  } else if (!ordered_task->is_max_size()) {

    AbstractTaskFactory &factory = ordered_task->get_factory();
    const Waypoint* way_point =
        dlgWaypointSelect(wf->GetMainWindow(),
                          ordered_task->task_size() > 0 ?
                          ordered_task->get_tp(0)->get_location() :
                          XCSoarInterface::Basic().location);
    if (!way_point)
      return;

    if (factory.append_optional_start(*way_point)) {
      task_modified = true;
    }
  }
  RefreshView();
}

static void
OnRelocateClicked(gcc_unused WndButton &Sender)
{
  OnOptionalStartListEnter(wOptionalStartPoints->GetCursorIndex());
  RefreshView();
}

static void
OnRemoveClicked(gcc_unused WndButton &Sender)
{
  const unsigned index_optional_starts = wOptionalStartPoints->GetCursorIndex()
      - (RealStartExists ? 1 : 0);
  if (ordered_task->remove_optional_start(index_optional_starts)) {
    RefreshView();
    task_modified = true;
  }
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnRemoveClicked),
  DeclareCallBackEntry(OnRelocateClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskOptionalStarts(SingleWindow &parent, OrderedTask** task)
{
  ordered_task = *task;
  task_modified = false;

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable, parent, _T("IDR_XML_TASKOPTIONALSTARTS_L"));
  else
    wf = LoadDialog(CallBackTable, parent, _T("IDR_XML_TASKOPTIONALSTARTS"));

  if (!wf)
    return false;

  assert(wf != NULL);

  wOptionalStartPoints = (WndListFrame*)wf->FindByName(_T("frmOptionalStarts"));
  assert(wOptionalStartPoints != NULL);

  RealStartExists = ordered_task->task_size();

  wOptionalStartPoints->SetActivateCallback(OnOptionalStartListEnter);
  wOptionalStartPoints->SetPaintItemCallback(OnOptionalStartPaintListItem);

  RefreshView();

  wf->ShowModal();

  delete wf;
  wf = NULL;

  return task_modified;
}
