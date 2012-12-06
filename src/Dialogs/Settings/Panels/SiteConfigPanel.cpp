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

#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Form/Button.hpp"
#include "LocalPath.hpp"
#include "Protection.hpp"
#include "ConfigPanel.hpp"
#include "SiteConfigPanel.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  DataPath,
  MapFile,
  WaypointFile,
  AdditionalWaypointFile,
  WatchedWaypointFile,
  AirspaceFile,
  AdditionalAirspaceFile,
  AirfieldFile
};

class SiteConfigPanel : public RowFormWidget {
public:
  SiteConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()), buttonWaypoints(0) {}

private:
  WndButton *buttonWaypoints;

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
SiteConfigPanel::Show(const PixelRect &rc)
{
  buttonWaypoints->SetVisible(true);
  RowFormWidget::Show(rc);
}

void
SiteConfigPanel::Hide()
{
  buttonWaypoints->SetVisible(false);
  RowFormWidget::Hide();
}

void
SiteConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  buttonWaypoints = ((WndButton *)ConfigPanel::GetForm().FindByName(_T("cmdWaypoints")));
  assert (buttonWaypoints);
  buttonWaypoints->SetOnClickNotify(dlgConfigWaypointsShowModal);

  WndProperty *wp = Add(_T(""), 0, true);
  wp->SetText(GetPrimaryDataPath());
  wp->SetEnabled(false);

  AddFileReader(_("Map database"),
                _("The name of the file (.xcm) containing terrain, topography, and optionally "
                    "waypoints, their details and airspaces."),
                ProfileKeys::MapFile, _T("*.xcm\0*.lkm\0"));

  AddFileReader(_("Waypoints"),
                _("Primary waypoints file.  Supported file types are Cambridge/WinPilot files (.dat), "
                    "Zander files (.wpz) or SeeYou files (.cup)."),
                ProfileKeys::WaypointFile, _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));

  AddFileReader(_("More waypoints"),
                _("Secondary waypoints file.  This may be used to add waypoints for a competition."),
                ProfileKeys::AdditionalWaypointFile, _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));
  SetExpertRow(AdditionalWaypointFile);

  AddFileReader(_("Watched waypoints"),
                _("Waypoint file containing special waypoints for which additional computations like "
                    "calculation of arrival height in map display always takes place. Useful for "
                    "waypoints like known reliable thermal sources (e.g. powerplants) or mountain passes."),
                ProfileKeys::WatchedWaypointFile, _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));
  SetExpertRow(WatchedWaypointFile);

  AddFileReader(_("Airspaces"), _("The file name of the primary airspace file."),
                ProfileKeys::AirspaceFile, _T("*.txt\0*.air\0*.sua\0"));

  AddFileReader(_("More airspaces"), _("The file name of the secondary airspace file."),
                ProfileKeys::AdditionalAirspaceFile, _T("*.txt\0*.air\0*.sua\0"));
  SetExpertRow(AdditionalAirspaceFile);

  AddFileReader(_("Waypoint details"),
                _("The file may contain extracts from enroute supplements or other contributed "
                    "information about individual waypoints and airfields."),
                ProfileKeys::AirfieldFile, _T("*.txt\0"));
  SetExpertRow(AirfieldFile);
}

bool
SiteConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  MapFileChanged = SaveValueFileReader(MapFile, ProfileKeys::MapFile);

  // WaypointFileChanged has already a meaningful value
  WaypointFileChanged |= SaveValueFileReader(WaypointFile, ProfileKeys::WaypointFile);
  WaypointFileChanged |= SaveValueFileReader(AdditionalWaypointFile, ProfileKeys::AdditionalWaypointFile);
  WaypointFileChanged |= SaveValueFileReader(WatchedWaypointFile, ProfileKeys::WatchedWaypointFile);

  AirspaceFileChanged = SaveValueFileReader(AirspaceFile, ProfileKeys::AirspaceFile);
  AirspaceFileChanged |= SaveValueFileReader(AdditionalAirspaceFile, ProfileKeys::AdditionalAirspaceFile);

  AirfieldFileChanged = SaveValueFileReader(AirfieldFile, ProfileKeys::AirfieldFile);


  changed = WaypointFileChanged || AirfieldFileChanged || MapFileChanged;

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateSiteConfigPanel()
{
  return new SiteConfigPanel();
}
