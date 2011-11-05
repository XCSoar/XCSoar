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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Waypoint.hpp"
#include "Form/Edit.hpp"
#include "LocalPath.hpp"
#include "Protection.hpp"
#include "ConfigPanel.hpp"
#include "SiteConfigPanel.hpp"

static WndForm* wf = NULL;
static WndButton *buttonWaypoints = NULL;

using namespace ConfigPanel;


void
SiteConfigPanel::SetVisible(bool active)
{
  if (buttonWaypoints != NULL)
    buttonWaypoints->set_visible(active);
}

bool
SiteConfigPanel::PreShow()
{
  SiteConfigPanel::SetVisible(true);
  return true;
}

bool
SiteConfigPanel::PreHide()
{
  SiteConfigPanel::SetVisible(false);
  return true;
}

static void
OnWaypoints(gcc_unused WndButton &button)
{
  dlgConfigWaypointsShowModal();
}


void
SiteConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  buttonWaypoints = ((WndButton *)wf->FindByName(_T("cmdWaypoints")));
  if (buttonWaypoints)
    buttonWaypoints->SetOnClickNotify(OnWaypoints);

  InitFileField(*wf, _T("prpAirspaceFile"),
                szProfileAirspaceFile, _T("*.txt\0*.air\0*.sua\0"));
  InitFileField(*wf, _T("prpAdditionalAirspaceFile"),
                szProfileAdditionalAirspaceFile, _T("*.txt\0*.air\0*.sua\0"));
  InitFileField(*wf, _T("prpWaypointFile"),
                szProfileWaypointFile, _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));
  InitFileField(*wf, _T("prpAdditionalWaypointFile"),
                szProfileAdditionalWaypointFile,
                _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));
  InitFileField(*wf, _T("prpWatchedWaypointFile"),
                szProfileWatchedWaypointFile,
                _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));

  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpDataPath"));
  wp->set_enabled(false);
  wp->SetText(GetPrimaryDataPath());

  InitFileField(*wf, _T("prpMapFile"),
                szProfileMapFile, _T("*.xcm\0*.lkm\0"));
  InitFileField(*wf, _T("prpTerrainFile"),
                szProfileTerrainFile, _T("*.jp2\0"));
  InitFileField(*wf, _T("prpTopographyFile"),
                szProfileTopographyFile, _T("*.tpl\0"));
  InitFileField(*wf, _T("prpAirfieldFile"),
                szProfileAirfieldFile, _T("*.txt\0"));
}


bool
SiteConfigPanel::Save()
{
  WaypointFileChanged = WaypointFileChanged |
    FinishFileField(*wf, _T("prpWaypointFile"), szProfileWaypointFile) |
    FinishFileField(*wf, _T("prpAdditionalWaypointFile"),
                    szProfileAdditionalWaypointFile) |
    FinishFileField(*wf, _T("prpWatchedWaypointFile"),
                    szProfileWatchedWaypointFile);

  AirspaceFileChanged =
    FinishFileField(*wf, _T("prpAirspaceFile"), szProfileAirspaceFile) |
    FinishFileField(*wf, _T("prpAdditionalAirspaceFile"),
                    szProfileAdditionalAirspaceFile);

  MapFileChanged = FinishFileField(*wf, _T("prpMapFile"), szProfileMapFile);

  TerrainFileChanged = FinishFileField(*wf, _T("prpTerrainFile"),
                                       szProfileTerrainFile);

  TopographyFileChanged = FinishFileField(*wf, _T("prpTopographyFile"),
                                        szProfileTopographyFile);

  AirfieldFileChanged = FinishFileField(*wf, _T("prpAirfieldFile"),
                                        szProfileAirfieldFile);


  return WaypointFileChanged || AirfieldFileChanged || MapFileChanged ||
         TerrainFileChanged || TopographyFileChanged || AirfieldFileChanged;
}
