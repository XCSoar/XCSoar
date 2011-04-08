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
#include "Protection.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Components.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WayPointFile.hpp"
#include "WayPoint/WayPointGlue.hpp"
#include "Compiler.h"

#include <assert.h>

static bool WaypointsNeedSave = false;
static WndForm *wf = NULL;

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnWaypointNewClicked(WndButton &Sender)
{
  (void)Sender;

  Waypoint edit_waypoint = way_points.create(XCSoarInterface::Basic().Location);
  edit_waypoint.Altitude = XCSoarInterface::Calculated().TerrainValid
    ? XCSoarInterface::Calculated().TerrainAlt
    : XCSoarInterface::Basic().NavAltitude;

  if (dlgWaypointEditShowModal(edit_waypoint) &&
      edit_waypoint.Name.size()) {
    way_points.append(edit_waypoint);
    WaypointsNeedSave = true;
  }
}

static void
OnWaypointEditClicked(WndButton &Sender)
{
  (void)Sender;

  const Waypoint *way_point = dlgWayPointSelect(XCSoarInterface::main_window,
                                                XCSoarInterface::Basic().Location);
  if (way_point) {
    Waypoint wp_copy = *way_point;
    if (dlgWaypointEditShowModal(wp_copy)) {
      WaypointsNeedSave = true;
      way_points.replace(*way_point, wp_copy);
    }
  }
}

static void
SaveWaypoints()
{
  way_points.optimise();
  if (!WayPointGlue::SaveWaypoints(way_points))
    MessageBoxX(_("Waypoints not editable"), _("Error"), MB_OK);
  else
    WaypointFileChanged = true;

  WaypointsNeedSave = false;
}

static void
OnWaypointSaveClicked(WndButton &Sender)
{
  (void)Sender;
  SaveWaypoints();
}

static void
OnWaypointDeleteClicked(WndButton &Sender)
{
  (void)Sender;
#ifdef OLD_TASK
  int res;
  res = dlgWayPointSelect(XCSoarInterface::Basic().Location);
  if (res != -1){
    if(MessageBoxX(way_points.get(res).Name,
                   _("Delete Waypoint?"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES) {
      Waypoint &waypoint = way_points.set(res);

      waypoint.FileNum = -1;
      waypoint.original_id = 0;
      WaypointsNeedSave = true;
    }
  }
#endif
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnWaypointNewClicked),
  DeclareCallBackEntry(OnWaypointDeleteClicked),
  DeclareCallBackEntry(OnWaypointEditClicked),
  DeclareCallBackEntry(OnWaypointSaveClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgConfigWaypointsShowModal()
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_CONFIG_WAYPOINTS_L") :
                                      _T("IDR_XML_CONFIG_WAYPOINTS"));
  assert(wf != NULL);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  WaypointsNeedSave = false;

  wf->ShowModal();

  if (WaypointsNeedSave &&
      MessageBoxX(_("Save changes to waypoint file?"), _("Waypoints edited"),
                  MB_YESNO | MB_ICONQUESTION) == IDYES)
      SaveWaypoints();

  delete wf;
}
