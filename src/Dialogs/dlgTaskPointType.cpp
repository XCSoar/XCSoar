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

#include "Dialogs/Task.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "LocalPath.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Factory/TaskFactoryConstraints.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h>

static WndForm *wf = NULL;
static ListControl* wPointTypes = NULL;
static bool task_modified = false;
static OrderedTask* ordered_task = NULL;
static OrderedTaskPoint* point = NULL;
static unsigned active_index = 0;
static const Waypoint* way_point = NULL;

static AbstractTaskFactory::LegalPointVector point_types;

static void OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static TaskPointFactoryType
get_point_type() 
{
  return ordered_task->GetFactory().GetType(*point);
}


static TaskPointFactoryType
get_cursor_type() 
{
  return point_types[wPointTypes->GetCursorIndex()];
}

static void
RefreshView()
{
  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  if (wSummary) {
    const TCHAR* text = OrderedTaskPointDescription(get_cursor_type());
    wSummary->SetCaption(text);
  }
}


static void
OnPointPaintListItem(Canvas &canvas, const PixelRect rc,
                     unsigned DrawListIndex)
{
  assert(DrawListIndex < point_types.size());

  StaticString<120> buffer;

  const TCHAR* text = OrderedTaskPointName(point_types[DrawListIndex]);

  if (point && (point_types[DrawListIndex] == get_point_type()))
    buffer.Format(_T("*%s"), text);
  else
    buffer.Format(_T(" %s"), text);

  canvas.text(rc.left + Layout::FastScale(2),
              rc.top + Layout::FastScale(2), buffer);
}

static bool
SetPointType(TaskPointFactoryType type)
{
  bool apply = false;

  if (!point) {
    apply = true;
    // empty point, don't ask confirmation
  } else {
    if (type == get_point_type())
      // no change
      return true;

    if (ShowMessageBox(_("Change point type?"), _("Task Point"),
                    MB_YESNO | MB_ICONQUESTION) == IDYES)
      apply = true;
  }

  if (apply) {
    AbstractTaskFactory &factory = ordered_task->GetFactory();

    if (point) {
      point = factory.CreateMutatedPoint(*point, type);
      if (point == NULL)
        return false;

      if (factory.Replace(*point, active_index, true))
        task_modified = true;
      delete point;
    } else {
      if (factory.IsValidFinishType(type) &&
          ordered_task->GetFactoryConstraints().is_closed)
        way_point = &ordered_task->GetPoint(0).GetWaypoint();
      else {
        const GeoPoint &location = ordered_task->TaskSize() > 0
          ? ordered_task->GetPoint(ordered_task->TaskSize() - 1).GetLocation()
          : CommonInterface::Basic().location;
        way_point =
          ShowWaypointListDialog(wf->GetMainWindow(), location);
      }
      if (!way_point)
        return false;

      point = factory.CreatePoint(type, *way_point);
      if (point == NULL)
        return false;

      if (factory.Append(*point, true))
        task_modified = true;

      delete point;
    }
    return true;
  }
  return false;
}

static void
OnSelect()
{
  if (wPointTypes->GetCursorIndex() >= point_types.size())
    return;

  if (SetPointType(get_cursor_type()))
    wf->SetModalResult(mrOK);
  else
    wf->SetModalResult(mrCancel);
}

static void 
OnSelectClicked(gcc_unused WndButton &Sender)
{
  OnSelect();
}

static void
OnPointListEnter(gcc_unused unsigned ItemIndex)
{
  OnSelect();
}

static void
OnPointCursorCallback(gcc_unused unsigned i)
{
  RefreshView();
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnSelectClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskPointNew(SingleWindow &parent, OrderedTask** task, const unsigned index)
{
  return dlgTaskPointType(parent, task, index);
}

bool
dlgTaskPointType(SingleWindow &parent, OrderedTask** task, const unsigned index)
{
  ordered_task = *task;
  task_modified = false;
  active_index = index;

  point = &ordered_task->GetPoint(active_index);
  way_point = &point->GetWaypoint();

  point_types = ordered_task->GetFactory().GetValidTypes(index);
  if (point_types.empty()) {
    assert(1);
    return false;
  }

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable, parent, _T("IDR_XML_TASKPOINTTYPE_L"));
  else
    wf = LoadDialog(CallBackTable, parent, _T("IDR_XML_TASKPOINTTYPE"));

  if (!wf)
    return false;

  assert(wf != NULL);

  wPointTypes = (ListControl*)wf->FindByName(_T("frmPointTypes"));
  assert(wPointTypes != NULL);

  wPointTypes->SetActivateCallback(OnPointListEnter);
  wPointTypes->SetPaintItemCallback(OnPointPaintListItem);
  wPointTypes->SetCursorCallback(OnPointCursorCallback);
  wPointTypes->SetLength(point_types.size());

  if (point)
    for (unsigned i=0; i<point_types.size(); i++)
      if (point_types[i] == get_point_type())
        wPointTypes->SetCursorIndex(i); 

  RefreshView();

  if (point_types.size()==1)
    SetPointType(point_types[0]);
  else
    wf->ShowModal();

  delete wf;
  wf = NULL;

  return task_modified;
}
