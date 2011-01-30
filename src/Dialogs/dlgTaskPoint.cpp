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
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Components.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "DataField/Float.hpp"

#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Gauge/TaskView.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"

#include <assert.h>
#include <stdio.h>

static SingleWindow *parent_window;
static WndForm *wf = NULL;
static WndFrame* wTaskView = NULL;
static OrderedTask* ordered_task = NULL;
static bool task_modified = false;
static unsigned active_index = 0;
static int next_previous = 0;

static void
OnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}


class TPLabelObservationZone:
  public ObservationZoneConstVisitor
{
public:
  void
  Visit(const FAISectorZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZFAISector")));
    if (wp)
      wp->show();
  }
  void
  Visit(const KeyholeZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZKeyhole")));
    if (wp)
      wp->show();
  }

  void
  Visit(const BGAFixedCourseZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZBGAFixedCourse")));
    if (wp)
      wp->show();
  }

  void
  Visit(const BGAEnhancedOptionZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZBGAEnhancedOption")));
    if (wp)
      wp->show();
  }

  void
  Visit(const BGAStartSectorZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZBGAStartSector")));
    if (wp)
      wp->show();
  }

  void
  Visit(const SectorZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZSector")));
    if (wp)
      wp->show();

    LoadFormProperty(*wf, _T("prpOZSectorRadius"),
                     ugDistance, oz.getRadius());
    LoadFormProperty(*wf, _T("prpOZSectorStartRadial"),
                     oz.getStartRadial().value_degrees());
    LoadFormProperty(*wf, _T("prpOZSectorFinishRadial"),
                     oz.getEndRadial().value_degrees());
  }

  void
  Visit(const LineSectorZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZLine")));
    if (wp)
      wp->show();

    LoadFormProperty(*wf, _T("prpOZLineLength"),
                     ugDistance, oz.getLength());
  }

  void
  Visit(const CylinderZone& oz)
  {
    hide_all();
    WndFrame* wp = ((WndFrame *)wf->FindByName(_T("frmOZCylinder")));
    if (wp)
      wp->show();

    LoadFormProperty(*wf, _T("prpOZCylinderRadius"),
                     ugDistance, oz.getRadius());
  }

private:
  void
  hide_all()
  {
    WndFrame* wp;
    wp = ((WndFrame *)wf->FindByName(_T("frmOZFAISector")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZKeyhole")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZBGAFixedCourse")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZBGAEnhancedOption")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZBGAStartSector")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZLine")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZSector")));
    if (wp)
      wp->hide();

    wp = ((WndFrame *)wf->FindByName(_T("frmOZCylinder")));
    if (wp)
      wp->hide();
  }
};

/**
 * Utility class to read observation zone parameters and update the dlgTaskPoint dialog
 * items
 */
class TPReadObservationZone:
  public ObservationZoneVisitor
{
public:
  void
  Visit(FAISectorZone& oz)
  {
  }
  void
  Visit(KeyholeZone& oz)
  {
  }
  void
  Visit(BGAFixedCourseZone& oz)
  {
  }
  void
  Visit(BGAEnhancedOptionZone& oz)
  {
  }
  void
  Visit(BGAStartSectorZone& oz)
  {
  }
  void
  Visit(SectorZone& oz)
  {
    fixed radius =
      Units::ToSysDistance(GetFormValueFixed(*wf, _T("prpOZSectorRadius")));
    if (fabs(radius - oz.getRadius()) > fixed(49)) {
      oz.setRadius(radius);
      task_modified = true;
    }

    fixed start_radial = GetFormValueFixed(*wf, _T("prpOZSectorStartRadial"));
    if (start_radial != oz.getStartRadial().value_degrees()) {
      oz.setStartRadial(Angle::degrees(start_radial));
      task_modified = true;
    }

    fixed finish_radial = GetFormValueFixed(*wf, _T("prpOZSectorFinishRadial"));
    if (finish_radial != oz.getEndRadial().value_degrees()) {
      oz.setEndRadial(Angle::degrees(finish_radial));
      task_modified = true;
    }
  }

  void
  Visit(LineSectorZone& oz)
  {
    fixed line_length =
      Units::ToSysDistance(GetFormValueFixed(*wf, _T("prpOZLineLength")));
    if (fabs(line_length - oz.getLength()) > fixed(49)) {
      oz.setLength(line_length);
      task_modified = true;
    }
  }

  void
  Visit(CylinderZone& oz)
  {
    fixed radius =
      Units::ToSysDistance(GetFormValueFixed(*wf, _T("prpOZCylinderRadius")));
    if (fabs(radius - oz.getRadius()) > fixed(49)) {
      oz.setRadius(radius);
      task_modified = true;
    }
  }
};

/**
 * Utility class to find labels for the task point
 */
class TPLabelTaskPoint:
  public TaskPointConstVisitor
{
public:
  TPLabelTaskPoint(TCHAR* buff):
    text(buff)
  {
    text[0] = _T('\0');
  }

  void Visit(const UnorderedTaskPoint& tp) {}
  void Visit(const StartPoint& tp) {    
    _tcscpy(text, _T("Start point"));
  }
  void Visit(const FinishPoint& tp) {
    _tcscpy(text, _T("Finish point"));
  }
  void Visit(const AATPoint& tp) {
    _tcscpy(text, _T("Assigned area point"));
  }
  void Visit(const ASTPoint& tp) {
    _tcscpy(text, _T("Task point"));
  }
  TCHAR* text;
};

static void
RefreshView()
{
  wTaskView->invalidate();

  OrderedTaskPoint* tp = ordered_task->get_tp(active_index);
  if (!tp)
    return;

  TPLabelObservationZone ozv;
  ObservationZoneConstVisitor &visitor = ozv;
  visitor.Visit(*tp->get_oz());

  WndFrame* wfrm = NULL;

  wfrm = ((WndFrame*)wf->FindByName(_T("lblType")));
  if (wfrm)
    wfrm->SetCaption(OrderedTaskPointName(ordered_task->get_factory().getType(*tp)));

  wfrm = ((WndFrame*)wf->FindByName(_T("lblLocation")));
  if (wfrm)
    wfrm->SetCaption(tp->get_waypoint().Name.c_str());

  WndButton* wb;
  wb = ((WndButton*)wf->FindByName(_T("butPrevious")));
  if (wb)
    wb->set_enabled(active_index > 0);

  wb = ((WndButton*)wf->FindByName(_T("butNext")));
  if (wb)
    wb->set_enabled(active_index < (ordered_task->task_size() - 1));

  TCHAR buf[100];
  TPLabelTaskPoint tpv(buf);
  TaskPointConstVisitor &tp_visitor = tpv;
  tp_visitor.Visit(*tp);
  wf->SetCaption(tpv.text);
}

static void
ReadValues()
{
  TPReadObservationZone tpv;
  OrderedTaskPoint* tp = ordered_task->get_tp(active_index);
  ObservationZoneVisitor &visitor = tpv;
  visitor.Visit(*tp->get_oz());
}

static void
OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  RECT rc = Sender->get_client_rect();

  OrderedTaskPoint* tp = ordered_task->get_tp(active_index);

  if (!tp) {
    canvas.clear_white();
    return;
  }

  PaintTaskPoint(canvas, rc, *ordered_task, *tp,
                 XCSoarInterface::Basic().Location,
                 XCSoarInterface::SettingsMap(),
                 terrain);
}

static void 
OnRemoveClicked(WndButton &Sender) 
{
  (void)Sender;

    if (MessageBoxX(_("Remove task point?"),
                          _("Task Point"), MB_YESNO | MB_ICONQUESTION) == IDNO)
      return;

  if (!ordered_task->get_factory().remove(active_index))
    return;

  task_modified = true;
  wf->SetModalResult(mrCancel);
}

static void
OnDetailsClicked(WndButton &Sender)
{
  OrderedTaskPoint* task_point = ordered_task->get_tp(active_index);
  if (task_point)
    dlgWayPointDetailsShowModal(*parent_window, task_point->get_waypoint(), false);
}

static void
OnRelocateClicked(WndButton &Sender)
{
  const Waypoint *wp = dlgWayPointSelect(*parent_window,
                                         XCSoarInterface::Basic().Location);
  if (wp == NULL)
    return;

  ordered_task->get_factory().relocate(active_index, *wp);
  task_modified = true;
  RefreshView();
}

static void
OnTypeClicked(WndButton &Sender)
{
  if (dlgTaskPointType(*parent_window, &ordered_task, active_index)) {
    task_modified = true;
    RefreshView();
  }
}

static void
OnPreviousClicked(WndButton &Sender)
{
  (void)Sender;
  if (active_index > 0) {
    next_previous=-1;
    wf->SetModalResult(mrOK);
  }
}

static void
OnNextClicked(WndButton &Sender)
{
  (void)Sender;
  if (active_index < (ordered_task->task_size() - 1)) {
    next_previous=1;
    wf->SetModalResult(mrOK);
  }
}

static void
OnOZLineLengthData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
 ReadValues();
}

static void
OnOZCylinderRadiusData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
 ReadValues();
}

static void
OnOZSectorRadiusData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
 ReadValues();
}

static void
OnOZSectorStartRadialData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
 ReadValues();
}

static void
OnOZSectorFinishRadialData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
 ReadValues();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnRemoveClicked),
  DeclareCallBackEntry(OnRelocateClicked),
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnTypeClicked),
  DeclareCallBackEntry(OnPreviousClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(OnOZLineLengthData),
  DeclareCallBackEntry(OnOZCylinderRadiusData),
  DeclareCallBackEntry(OnOZSectorRadiusData),
  DeclareCallBackEntry(OnOZSectorStartRadialData),
  DeclareCallBackEntry(OnOZSectorFinishRadialData),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskPointShowModal(SingleWindow &parent, OrderedTask** task,
                      const unsigned index)
{
  ordered_task = *task;
  parent_window = &parent;
  task_modified = false;
  active_index = index;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_TASKPOINT_L") :
                                      _T("IDR_XML_TASKPOINT"));
  assert(wf != NULL);

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);

  WndFrame* wType = (WndFrame*) wf->FindByName(_T("lblType"));
  assert (wType);
  wType->SetFont(Fonts::MapBold);

  WndFrame* wLocation = (WndFrame*) wf->FindByName(_T("lblLocation"));
  assert (wLocation);
  wLocation->SetFont(Fonts::MapBold);

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
    ordered_task->update_geometry();
  }
  return task_modified;
}
