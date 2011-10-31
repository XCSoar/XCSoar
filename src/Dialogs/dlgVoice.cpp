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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Units/Units.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Audio/VegaVoice.hpp"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"

static WndForm *wf=NULL;

static void OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}


static gcc_constexpr_data CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


static void LoadIntoForm(WndForm &form, const SETTINGS_COMPUTER &settings){
  LoadFormProperty(form, _T("prpVoiceClimbRate"),
                   settings.voice_climb_rate_enabled);
  LoadFormProperty(form, _T("prpVoiceTerrain"), settings.voice_terrain_enabled);
  LoadFormProperty(form, _T("prpVoiceWaypointDistance"),
                   settings.voice_waypoint_distance_enabled);
  LoadFormProperty(form, _T("prpVoiceTaskAltitudeDifference"),
                   settings.voice_task_altitude_difference_enabled);
  LoadFormProperty(form, _T("prpVoiceMacCready"),
                   settings.voice_mac_cready_enabled);
  LoadFormProperty(form, _T("prpVoiceNewWaypoint"),
                   settings.voice_new_waypoint_enabled);
  LoadFormProperty(form, _T("prpVoiceInSector"), settings.voice_in_sector_enabled);
  LoadFormProperty(form, _T("prpVoiceAirspace"), settings.voice_airspace_enabled);
}

static bool
SaveFromForm(const WndForm &form, SETTINGS_COMPUTER &settings)
{
  return
    SaveFormProperty(form, _T("prpVoiceClimbRate"),
                     settings.voice_climb_rate_enabled,
                     szProfileVoiceClimbRate) ||
    SaveFormProperty(form, _T("prpVoiceTerrain"),
                     settings.voice_terrain_enabled, szProfileVoiceTerrain) ||
    SaveFormProperty(form, _T("prpVoiceWaypointDistance"),
                     settings.voice_waypoint_distance_enabled,
                     szProfileVoiceWaypointDistance) ||
    SaveFormProperty(form, _T("prpVoiceTaskAltitudeDifference"),
                     settings.voice_task_altitude_difference_enabled,
                     szProfileVoiceTaskAltitudeDifference) ||
    SaveFormProperty(form, _T("prpVoiceMacCready"),
                     settings.voice_mac_cready_enabled,
                     szProfileVoiceMacCready) ||
    SaveFormProperty(form, _T("prpVoiceNewWaypoint"),
                     settings.voice_new_waypoint_enabled,
                     szProfileVoiceNewWaypoint) ||
    SaveFormProperty(form, _T("prpVoiceInSector"),
                     settings.voice_in_sector_enabled, szProfileVoiceInSector) ||
    SaveFormProperty(form, _T("prpVoiceAirspace"),
                     settings.voice_airspace_enabled, szProfileVoiceAirspace);
}


void dlgVoiceShowModal(void){
  wf = LoadDialog(CallBackTable,
		      XCSoarInterface::main_window,
		      _T("IDR_XML_VOICE"));

  
  if (!wf) return;

  LoadIntoForm(*wf, XCSoarInterface::SettingsComputer());

  wf->ShowModal();

  bool changed = false;

  changed = SaveFromForm(*wf, XCSoarInterface::SetSettingsComputer());

  if (changed) {
    Profile::Save();
    LogDebug(_T("Voice configuration: Changes saved"));
  }

  delete wf;
  wf = NULL;

}

