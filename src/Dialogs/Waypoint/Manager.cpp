/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Protection.hpp"
#include "UtilsSettings.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class WaypointManagerWidget final
  : public RowFormWidget, private ActionListener {
  enum Buttons {
    NEW,
    EDIT,
    SAVE,
    DELETE,
  };

public:
  WaypointManagerWidget(const DialogLook &look)
    :RowFormWidget(look) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* virtual methods from ActionListener */
  void OnAction(int id) override;
};

static bool WaypointsNeedSave = false;

void
WaypointManagerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddButton(_("New"), *this, NEW);
  AddButton(_("Edit"), *this, EDIT);
  AddButton(_("Save"), *this, SAVE);
  AddButton(_("Delete"), *this, DELETE);
}

static void
OnWaypointNewClicked()
{
  if (!WaypointGlue::IsWritable()) {
    ShowMessageBox(_("Waypoints not editable"), _("Error"), MB_OK);
    return;
  }

  Waypoint edit_waypoint = way_points.Create(CommonInterface::Basic().location);
  edit_waypoint.elevation = CommonInterface::Calculated().terrain_valid
    ? CommonInterface::Calculated().terrain_altitude
    : CommonInterface::Basic().nav_altitude;

  if (dlgWaypointEditShowModal(edit_waypoint) &&
      edit_waypoint.name.size()) {
    WaypointsNeedSave = true;

    ScopeSuspendAllThreads suspend;
    way_points.Append(std::move(edit_waypoint));
    way_points.Optimise();
  }
}

static void
OnWaypointEditClicked()
{
  if (!WaypointGlue::IsWritable()) {
    ShowMessageBox(_("Waypoints not editable"), _("Error"), MB_OK);
    return;
  }

  const Waypoint *way_point =
    ShowWaypointListDialog(CommonInterface::Basic().location);
  if (way_point) {
    Waypoint wp_copy = *way_point;
    if (dlgWaypointEditShowModal(wp_copy)) {
      WaypointsNeedSave = true;

      ScopeSuspendAllThreads suspend;
      way_points.Replace(*way_point, wp_copy);
      way_points.Optimise();
    }
  }
}

static void
SaveWaypoints()
{
  if (!WaypointGlue::SaveWaypoints(way_points))
    ShowMessageBox(_("Waypoints not editable"), _("Error"), MB_OK);
  else
    WaypointFileChanged = true;

  WaypointsNeedSave = false;
}

static void
OnWaypointSaveClicked()
{
  SaveWaypoints();
}

static void
OnWaypointDeleteClicked()
{
  if (!WaypointGlue::IsWritable()) {
    ShowMessageBox(_("Waypoints not editable"), _("Error"), MB_OK);
    return;
  }

  const Waypoint *way_point =
    ShowWaypointListDialog(CommonInterface::Basic().location);
  if (way_point == nullptr)
    return;

  if (ShowMessageBox(way_point->name.c_str(), _("Delete waypoint?"),
                     MB_YESNO | MB_ICONQUESTION) == IDYES) {
    WaypointsNeedSave = true;

    ScopeSuspendAllThreads suspend;
    way_points.Erase(*way_point);
    way_points.Optimise();
  }
}

void
WaypointManagerWidget::OnAction(int id)
{
  switch (Buttons(id)) {
  case NEW:
    OnWaypointNewClicked();
    break;

  case EDIT:
    OnWaypointEditClicked();
    break;

  case SAVE:
    OnWaypointSaveClicked();
    break;

  case DELETE:
    OnWaypointDeleteClicked();
    break;
  }
}

void
dlgConfigWaypointsShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Waypoints Editor"),
                    new WaypointManagerWidget(look));
  dialog.AddButton(_("Close"), mrCancel);

  WaypointsNeedSave = false;
  dialog.ShowModal();

  if (WaypointsNeedSave &&
      ShowMessageBox(_("Save changes to waypoint file?"), _("Waypoints edited"),
                  MB_YESNO | MB_ICONQUESTION) == IDYES)
      SaveWaypoints();
}
