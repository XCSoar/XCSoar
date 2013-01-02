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

#include "VoiceSettingsDialog.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Base.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Audio/VegaVoice.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"

static WndForm *wf=NULL;

static void OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}


static constexpr CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


static void LoadIntoForm(WndForm &form, const VoiceSettings &settings){
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
SaveFromForm(const WndForm &form, VoiceSettings &settings)
{
  return
    SaveFormProperty(form, _T("prpVoiceClimbRate"),
                     settings.voice_climb_rate_enabled,
                     ProfileKeys::VoiceClimbRate) ||
    SaveFormProperty(form, _T("prpVoiceTerrain"),
                     settings.voice_terrain_enabled, ProfileKeys::VoiceTerrain) ||
    SaveFormProperty(form, _T("prpVoiceWaypointDistance"),
                     settings.voice_waypoint_distance_enabled,
                     ProfileKeys::VoiceWaypointDistance) ||
    SaveFormProperty(form, _T("prpVoiceTaskAltitudeDifference"),
                     settings.voice_task_altitude_difference_enabled,
                     ProfileKeys::VoiceTaskAltitudeDifference) ||
    SaveFormProperty(form, _T("prpVoiceMacCready"),
                     settings.voice_mac_cready_enabled,
                     ProfileKeys::VoiceMacCready) ||
    SaveFormProperty(form, _T("prpVoiceNewWaypoint"),
                     settings.voice_new_waypoint_enabled,
                     ProfileKeys::VoiceNewWaypoint) ||
    SaveFormProperty(form, _T("prpVoiceInSector"),
                     settings.voice_in_sector_enabled, ProfileKeys::VoiceInSector) ||
    SaveFormProperty(form, _T("prpVoiceAirspace"),
                     settings.voice_airspace_enabled, ProfileKeys::VoiceAirspace);
}


void dlgVoiceShowModal(){
  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
		      _T("IDR_XML_VOICE"));

  
  if (!wf) return;

  LoadIntoForm(*wf, CommonInterface::GetComputerSettings().voice);

  wf->ShowModal();

  bool changed = false;

  changed = SaveFromForm(*wf, CommonInterface::SetComputerSettings().voice);
  delete wf;

  if (changed) {
    Profile::Save();
    LogDebug(_T("Voice configuration: Changes saved"));
  }
}
