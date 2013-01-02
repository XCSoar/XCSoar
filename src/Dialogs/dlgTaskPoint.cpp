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

#include "Dialogs/Task.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Draw.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Float.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Units/Units.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Gauge/TaskView.hpp"
#include "Compiler.h"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <assert.h>
#include <stdio.h>

static WndForm *wf = NULL;
static WndFrame* wTaskView = NULL;
static OrderedTask* ordered_task = NULL;
static bool task_modified = false;
static unsigned active_index = 0;
static int next_previous = 0;

// setting to True during refresh so control values don't trigger form save
static bool Refreshing = false;

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

/**
 * for FAI tasks, make the zone sizes disabled so the user can't alter them
 * @param enable
 */
static void
EnableSizeEdit(bool enable)
{
  SetFormControlEnabled(*wf, _T("prpOZLineLength"), enable);
  SetFormControlEnabled(*wf, _T("prpOZCylinderRadius"), enable);
}

static void
RefreshView()
{
  wTaskView->Invalidate();

  const OrderedTaskPoint &tp = ordered_task->GetPoint(active_index);

  Refreshing = true; // tell onChange routines not to save form!

  ShowFormControl(*wf, _T("frmOZLine"), false);
  ShowFormControl(*wf, _T("frmOZSector"), false);
  ShowFormControl(*wf, _T("frmOZCylinder"), false);

  const ObservationZonePoint &oz = tp.GetObservationZone();

  switch (oz.shape) {
  case ObservationZonePoint::SECTOR:
  case ObservationZonePoint::ANNULAR_SECTOR:
    ShowFormControl(*wf, _T("frmOZSector"), true);

    LoadFormProperty(*wf, _T("prpOZSectorRadius"),
                     UnitGroup::DISTANCE, ((const SectorZone &)oz).GetRadius());
    LoadFormProperty(*wf, _T("prpOZSectorStartRadial"),
                     ((const SectorZone &)oz).GetStartRadial().Degrees());
    LoadFormProperty(*wf, _T("prpOZSectorFinishRadial"),
                     ((const SectorZone &)oz).GetEndRadial().Degrees());

    if (oz.shape == ObservationZonePoint::ANNULAR_SECTOR) {
      LoadFormProperty(*wf, _T("prpOZSectorInnerRadius"),
                       UnitGroup::DISTANCE, ((const AnnularSectorZone &)oz).GetInnerRadius());

      ShowFormControl(*wf, _T("prpOZSectorInnerRadius"), true);
    } else
      ShowFormControl(*wf, _T("prpOZSectorInnerRadius"), false);

    break;

  case ObservationZonePoint::LINE:
    ShowFormControl(*wf, _T("frmOZLine"), true);

    LoadFormProperty(*wf, _T("prpOZLineLength"), UnitGroup::DISTANCE,
                     ((const LineSectorZone &)oz).GetLength());
    break;

  case ObservationZonePoint::CYLINDER:
    ShowFormControl(*wf, _T("frmOZCylinder"), true);

    LoadFormProperty(*wf, _T("prpOZCylinderRadius"), UnitGroup::DISTANCE,
                     ((const CylinderZone &)oz).GetRadius());
    break;

  default:
    break;
  }

  WndFrame* wfrm = NULL;
  wfrm = ((WndFrame*)wf->FindByName(_T("lblType")));
  if (wfrm)
    wfrm->SetCaption(OrderedTaskPointName(ordered_task->GetFactory().GetType(tp)));

  SetFormControlEnabled(*wf, _T("butPrevious"), active_index > 0);
  SetFormControlEnabled(*wf, _T("butNext"),
                        active_index < (ordered_task->TaskSize() - 1));

  WndButton* wb;
  wb = (WndButton*)wf->FindByName(_T("cmdOptionalStarts"));
  assert(wb);
  wb->SetVisible(active_index == 0);
  if (ordered_task->GetOptionalStartPointCount() == 0)
    wb->SetCaption(_("Enable Alternate Starts"));
  else {
    StaticString<50> tmp;
    tmp.Format(_T("%s (%d)"), _("Edit Alternates"),
               ordered_task->GetOptionalStartPointCount());
    wb->SetCaption(tmp);
  }

  EnableSizeEdit(ordered_task->GetFactoryType() != TaskFactoryType::FAI_GENERAL);

  StaticString<100> name_prefix_buffer, type_buffer;

  switch (tp.GetType()) {
  case TaskPoint::START:
    type_buffer = _T("Start point");
    name_prefix_buffer = _T("Start: ");
    break;

  case TaskPoint::AST:
    type_buffer = _T("Task point");
    name_prefix_buffer.Format(_T("%d: "), active_index);
    break;

  case TaskPoint::AAT:
    type_buffer = _T("Assigned area point");
    name_prefix_buffer.Format(_T("%d: "), active_index);
    break;

  case TaskPoint::FINISH:
    type_buffer = _T("Finish point");
    name_prefix_buffer = _T("Finish: ");
    break;

  default:
    assert(true);
    break;
  }

  wf->SetCaption(type_buffer);

  wfrm = ((WndFrame*)wf->FindByName(_T("lblLocation")));
  if (wfrm) {
    StaticString<100> buffer;
    buffer.Format(_T("%s %s"), name_prefix_buffer.c_str(),
                  tp.GetWaypoint().name.c_str());
    wfrm->SetCaption(buffer);
  }

  Refreshing = false; // reactivate onChange routines
}

static void
ReadValues()
{
  OrderedTaskPoint &tp = ordered_task->GetPoint(active_index);
  ObservationZonePoint &oz = tp.GetObservationZone();

  switch (oz.shape) {
  case ObservationZonePoint::ANNULAR_SECTOR: {
    fixed radius = Units::ToSysDistance(
        GetFormValueFixed(*wf, _T("prpOZSectorInnerRadius")));

    if (fabs(radius - ((AnnularSectorZone &)oz).GetInnerRadius()) > fixed(49)) {
      ((AnnularSectorZone &)oz).SetInnerRadius(radius);
      task_modified = true;
    }
  }
  case ObservationZonePoint::SECTOR: {
    fixed radius =
      Units::ToSysDistance(GetFormValueFixed(*wf, _T("prpOZSectorRadius")));

    if (fabs(radius - ((SectorZone &)oz).GetRadius()) > fixed(49)) {
      ((SectorZone &)oz).SetRadius(radius);
      task_modified = true;
    }

    fixed start_radial = GetFormValueFixed(*wf, _T("prpOZSectorStartRadial"));
    if (start_radial != ((SectorZone &)oz).GetStartRadial().Degrees()) {
      ((SectorZone &)oz).SetStartRadial(Angle::Degrees(start_radial));
      task_modified = true;
    }

    fixed finish_radial = GetFormValueFixed(*wf, _T("prpOZSectorFinishRadial"));
    if (finish_radial != ((SectorZone &)oz).GetEndRadial().Degrees()) {
      ((SectorZone &)oz).SetEndRadial(Angle::Degrees(finish_radial));
      task_modified = true;
    }
    break;
  }
  case ObservationZonePoint::LINE: {
    fixed line_length = Units::ToSysDistance(
        GetFormValueFixed(*wf, _T("prpOZLineLength")));

    if (fabs(line_length - ((LineSectorZone &)oz).GetLength()) > fixed(49)) {
      ((LineSectorZone &)oz).SetLength(line_length);
      task_modified = true;
    }
    break;
  }

  case ObservationZonePoint::CYLINDER: {
    fixed radius = Units::ToSysDistance(
        GetFormValueFixed(*wf, _T("prpOZCylinderRadius")));

    if (fabs(radius - ((CylinderZone &)oz).GetRadius()) > fixed(49)) {
      ((CylinderZone &)oz).SetRadius(radius);
      task_modified = true;
    }
    break;
  }

  default:
    break;
  }
}

static void
OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  PixelRect rc = Sender->GetClientRect();

  const OrderedTaskPoint &tp = ordered_task->GetPoint(active_index);

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  const MapLook &look = UIGlobals::GetMapLook();
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTaskPoint(canvas, rc, *ordered_task, tp,
                 basic.location_available, basic.location,
                 XCSoarInterface::GetMapSettings(),
                 look.task, look.airspace,
                 terrain, &airspace_database);
}

static void 
OnRemoveClicked(gcc_unused WndButton &Sender)
{
  if (ShowMessageBox(_("Remove task point?"), _("Task Point"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  if (!ordered_task->GetFactory().Remove(active_index))
    return;

  task_modified = true;
  wf->SetModalResult(mrCancel);
}

static void
OnDetailsClicked(gcc_unused WndButton &Sender)
{
  const OrderedTaskPoint &task_point = ordered_task->GetPoint(active_index);
  dlgWaypointDetailsShowModal(wf->GetMainWindow(),
                              task_point.GetWaypoint(), false);
}

static void
OnRelocateClicked(gcc_unused WndButton &Sender)
{
  const GeoPoint &gpBearing = active_index > 0
    ? ordered_task->GetPoint(active_index - 1).GetLocation()
    : CommonInterface::Basic().location;

  const Waypoint *wp = ShowWaypointListDialog(wf->GetMainWindow(), gpBearing,
                                         ordered_task, active_index);
  if (wp == NULL)
    return;

  ordered_task->GetFactory().Relocate(active_index, *wp);
  task_modified = true;
  RefreshView();
}

static void
OnTypeClicked(gcc_unused WndButton &Sender)
{
  if (dlgTaskPointType(wf->GetMainWindow(), &ordered_task, active_index)) {
    task_modified = true;
    RefreshView();
  }
}

static void
OnPreviousClicked(gcc_unused WndButton &Sender)
{
  if (active_index > 0) {
    next_previous=-1;
    wf->SetModalResult(mrOK);
  }
}

static void
OnNextClicked(gcc_unused WndButton &Sender)
{
  if (active_index < (ordered_task->TaskSize() - 1)) {
    next_previous=1;
    wf->SetModalResult(mrOK);
  }
}

/**
 * displays dlgTaskOptionalStarts
 * @param Sender
 */
static void
OnOptionalStartsClicked(gcc_unused WndButton &Sender)
{
  if (dlgTaskOptionalStarts(wf->GetMainWindow(), &ordered_task)) {
    task_modified = true;
    RefreshView();
  }
}

static void
OnOZData(gcc_unused DataField *Sender,
         gcc_unused DataField::DataAccessMode Mode)
{
  if (!Refreshing)
    ReadValues();
  wTaskView->Invalidate();
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnRemoveClicked),
  DeclareCallBackEntry(OnRelocateClicked),
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnTypeClicked),
  DeclareCallBackEntry(OnPreviousClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnOptionalStartsClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(OnOZData),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskPointShowModal(SingleWindow &parent, OrderedTask** task,
                      const unsigned index)
{
  ordered_task = *task;
  task_modified = false;
  active_index = index;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_TASKPOINT_L") :
                                      _T("IDR_XML_TASKPOINT"));
  assert(wf != NULL);

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);

  const DialogLook &look = UIGlobals::GetDialogLook();

  WndFrame* wType = (WndFrame*) wf->FindByName(_T("lblType"));
  assert (wType);
  wType->SetFont(*look.caption.font);

  WndFrame* wLocation = (WndFrame*) wf->FindByName(_T("lblLocation"));
  assert (wLocation);
  wLocation->SetFont(*look.caption.font);

  do {
    RefreshView();
    next_previous=0;
    if (wf->ShowModal() == mrOK)
      ReadValues();
    active_index += next_previous;
  } while (next_previous != 0);

  delete wf;

  if (*task != ordered_task) {
    *task = ordered_task;
    task_modified = true;
  } 
  if (task_modified) {
    ordered_task->UpdateGeometry();
  }
  return task_modified;
}
