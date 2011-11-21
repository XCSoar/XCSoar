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

#include "InputEvents.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Message.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Util/Macros.hpp"
#include "Units/Units.hpp"
#include "Protection.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"

static void
trigger_redraw()
{
  if (!XCSoarInterface::Basic().location_available)
    TriggerGPSUpdate();
  TriggerMapUpdate();
}

void
InputEvents::eventSounds(const TCHAR *misc)
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();
 // bool OldEnableSoundVario = EnableSoundVario;

  if (_tcscmp(misc, _T("toggle")) == 0)
    settings_computer.sound_vario_enabled = !settings_computer.sound_vario_enabled;
  else if (_tcscmp(misc, _T("on")) == 0)
    settings_computer.sound_vario_enabled = true;
  else if (_tcscmp(misc, _T("off")) == 0)
    settings_computer.sound_vario_enabled = false;
  else if (_tcscmp(misc, _T("show")) == 0) {
    if (settings_computer.sound_vario_enabled)
      Message::AddMessage(_("Vario sounds on"));
    else
      Message::AddMessage(_("Vario sounds off"));
  }
  /*
  if (EnableSoundVario != OldEnableSoundVario) {
    VarioSound_EnableSound(EnableSoundVario);
  }
  */
}

void
InputEvents::eventSnailTrail(const TCHAR *misc)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("toggle")) == 0) {
    unsigned trail_length = (int)settings_map.trail_length;
    trail_length = (trail_length + 1u) % 4u;
    settings_map.trail_length = (TrailLength)trail_length;
  } else if (_tcscmp(misc, _T("off")) == 0)
    settings_map.trail_length = TRAIL_OFF;
  else if (_tcscmp(misc, _T("long")) == 0)
    settings_map.trail_length = TRAIL_LONG;
  else if (_tcscmp(misc, _T("short")) == 0)
    settings_map.trail_length = TRAIL_SHORT;
  else if (_tcscmp(misc, _T("full")) == 0)
    settings_map.trail_length = TRAIL_FULL;
  else if (_tcscmp(misc, _T("show")) == 0) {
    switch (settings_map.trail_length) {
    case TRAIL_OFF:
      Message::AddMessage(_("Snail trail off"));
      break;

    case TRAIL_LONG:
      Message::AddMessage(_("Long snail trail"));
      break;

    case TRAIL_SHORT:
      Message::AddMessage(_("Short snail trail"));
      break;

    case TRAIL_FULL:
      Message::AddMessage(_("Full snail trail"));
      break;
    }
  }

  ActionInterface::SendSettingsMap(true);
}

void
InputEvents::eventTerrainTopology(const TCHAR *misc)
{
  eventTerrainTopography(misc);
}

// Do JUST Terrain/Topography (toggle any, on/off any, show)
void
InputEvents::eventTerrainTopography(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("terrain toggle")) == 0)
    sub_TerrainTopography(-2);
  else if (_tcscmp(misc, _T("topography toggle")) == 0)
    sub_TerrainTopography(-3);
  else if (_tcscmp(misc, _T("topology toggle")) == 0)
    sub_TerrainTopography(-3);
  else if (_tcscmp(misc, _T("terrain on")) == 0)
    sub_TerrainTopography(3);
  else if (_tcscmp(misc, _T("terrain off")) == 0)
    sub_TerrainTopography(4);
  else if (_tcscmp(misc, _T("topography on")) == 0)
    sub_TerrainTopography(1);
  else if (_tcscmp(misc, _T("topography off")) == 0)
    sub_TerrainTopography(2);
  else if (_tcscmp(misc, _T("topology on")) == 0)
    sub_TerrainTopography(1);
  else if (_tcscmp(misc, _T("topology off")) == 0)
    sub_TerrainTopography(2);
  else if (_tcscmp(misc, _T("show")) == 0)
    sub_TerrainTopography(0);
  else if (_tcscmp(misc, _T("toggle")) == 0)
    sub_TerrainTopography(-1);

  XCSoarInterface::SendSettingsMap(true);
}

// Adjust audio deadband of internal vario sounds
// +: increases deadband
// -: decreases deadband
void
InputEvents::eventAudioDeadband(const TCHAR *misc)
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  if (_tcscmp(misc, _T("+"))) {
    if (settings_computer.sound_deadband >= 40)
      return;

    settings_computer.sound_deadband++;
  }
  if (_tcscmp(misc, _T("-"))) {
    if (settings_computer.sound_deadband <= 0)
      return;

    settings_computer.sound_deadband--;
  }

  /*
  VarioSound_SetVdead(SoundDeadband);
  */

  Profile::SetSoundSettings(); // save to registry

  // TODO feature: send to vario if available
}

// Bugs
// Adjusts the degradation of glider performance due to bugs
// up: increases the performance by 10%
// down: decreases the performance by 10%
// max: cleans the aircraft of bugs
// min: selects the worst performance (50%)
// show: shows the current bug degradation
void
InputEvents::eventBugs(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  GlidePolar &polar = CommonInterface::SetSettingsComputer().glide_polar_task;
  fixed BUGS = polar.GetBugs();
  fixed oldBugs = BUGS;

  if (_tcscmp(misc, _T("up")) == 0) {
    BUGS += fixed_one / 10;
    if (BUGS > fixed_one)
      BUGS = fixed_one;
  } else if (_tcscmp(misc, _T("down")) == 0) {
    BUGS -= fixed_one / 10;
    if (BUGS < fixed_half)
      BUGS = fixed_half;
  } else if (_tcscmp(misc, _T("max")) == 0)
    BUGS = fixed_one;
  else if (_tcscmp(misc, _T("min")) == 0)
    BUGS = fixed_half;
  else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%d"), (int)(BUGS * 100));
    Message::AddMessage(_("Bugs performance"), Temp);
  }

  if (BUGS != oldBugs) {
    polar.SetBugs(fixed(BUGS));
    protected_task_manager->SetGlidePolar(polar);
  }
}

// Ballast
// Adjusts the ballast setting of the glider
// up: increases ballast by 10%
// down: decreases ballast by 10%
// max: selects 100% ballast
// min: selects 0% ballast
// show: displays a status message indicating the ballast percentage
void
InputEvents::eventBallast(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  GlidePolar &polar = CommonInterface::SetSettingsComputer().glide_polar_task;
  fixed BALLAST = polar.GetBallast();
  fixed oldBallast = BALLAST;

  if (_tcscmp(misc, _T("up")) == 0) {
    BALLAST += fixed_one / 10;
    if (BALLAST >= fixed_one)
      BALLAST = fixed_one;
  } else if (_tcscmp(misc, _T("down")) == 0) {
    BALLAST -= fixed_one / 10;
    if (BALLAST < fixed_zero)
      BALLAST = fixed_zero;
  } else if (_tcscmp(misc, _T("max")) == 0)
    BALLAST = fixed_one;
  else if (_tcscmp(misc, _T("min")) == 0)
    BALLAST = fixed_zero;
  else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%d"), (int)(BALLAST * 100));
    Message::AddMessage(_("Ballast %"), Temp);
  }

  if (BALLAST != oldBallast) {
    polar.SetBallast(fixed(BALLAST));
    protected_task_manager->SetGlidePolar(polar);
  }
}

// ProfileLoad
// Loads the profile of the specified filename
void
InputEvents::eventProfileLoad(const TCHAR *misc)
{
  if (!string_is_empty(misc)) {
    Profile::LoadFile(misc);

    WaypointFileChanged = true;
    TerrainFileChanged = true;
    TopographyFileChanged = true;
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;
    PolarFileChanged = true;

    // assuming all is ok, we can...
    Profile::Use();
  }
}

// ProfileSave
// Saves the profile to the specified filename
void
InputEvents::eventProfileSave(const TCHAR *misc)
{
  if (!string_is_empty(misc))
    Profile::SaveFile(misc);
}

// AdjustForecastTemperature
// Adjusts the maximum ground temperature used by the convection forecast
// +: increases temperature by one degree celsius
// -: decreases temperature by one degree celsius
// show: Shows a status message with the current forecast temperature
void
InputEvents::eventAdjustForecastTemperature(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("+")) == 0)
    CommonInterface::SetSettingsComputer().forecast_temperature += fixed_one;
  else if (_tcscmp(misc, _T("-")) == 0)
    CommonInterface::SetSettingsComputer().forecast_temperature -= fixed_one;
  else if (_tcscmp(misc, _T("show")) == 0) {
    fixed temperature =
      CommonInterface::SettingsComputer().forecast_temperature;
    TCHAR Temp[100];
    _stprintf(Temp, _T("%f"),
              (double)Units::ToUserTemperature(temperature));
    Message::AddMessage(_("Forecast temperature"), Temp);
  }
}

void
InputEvents::eventDeclutterLabels(const TCHAR *misc)
{
  static const TCHAR *const msg[] = {N_("All"),
                                     N_("Task & Landables"),
                                     N_("Task"),
                                     N_("None")};
  static gcc_constexpr_data unsigned int n = ARRAY_SIZE(msg);
  static const TCHAR *const actions[n] = {_T("all"),
                                          _T("task+landables"),
                                          _T("task"),
                                          _T("none")};

  WaypointLabelSelection_t &wls =
    XCSoarInterface::SetSettingsMap().waypoint.label_selection;
  if (_tcscmp(misc, _T("toggle")) == 0)
    wls = (WaypointLabelSelection_t) ((wls + 1) %  n);
  else if (_tcscmp(misc, _T("show")) == 0 && (unsigned int) wls < n) {
    TCHAR tbuf[64];
    _stprintf(tbuf, _T("%s: %s"), _("Waypoint labels"), gettext(msg[wls]));
    Message::AddMessage(tbuf);
  }
  else {
    for (unsigned int i=0; i<n; i++)
      if (_tcscmp(misc, actions[i]) == 0)
        wls = (WaypointLabelSelection_t) i;
  }

  ActionInterface::SendSettingsMap(true);
}

void
InputEvents::eventAirspaceDisplayMode(const TCHAR *misc)
{
  AirspaceRendererSettings &settings =
    CommonInterface::SetSettingsMap().airspace;

  if (_tcscmp(misc, _T("all")) == 0)
    settings.altitude_mode = ALLON;
  else if (_tcscmp(misc, _T("clip")) == 0)
    settings.altitude_mode = CLIP;
  else if (_tcscmp(misc, _T("auto")) == 0)
    settings.altitude_mode = AUTO;
  else if (_tcscmp(misc, _T("below")) == 0)
    settings.altitude_mode = ALLBELOW;
  else if (_tcscmp(misc, _T("off")) == 0)
    settings.altitude_mode = ALLOFF;

  trigger_redraw();
}

void
InputEvents::eventOrientation(const TCHAR *misc)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("northup")) == 0) {
    settings_map.cruise_orientation = NORTHUP;
    settings_map.circling_orientation = NORTHUP;
  } else if (_tcscmp(misc, _T("northcircle")) == 0) {
    settings_map.cruise_orientation = TRACKUP;
    settings_map.circling_orientation = NORTHUP;
  } else if (_tcscmp(misc, _T("trackcircle")) == 0) {
    settings_map.cruise_orientation = NORTHUP;
    settings_map.circling_orientation = TRACKUP;
  } else if (_tcscmp(misc, _T("trackup")) == 0) {
    settings_map.cruise_orientation = TRACKUP;
    settings_map.circling_orientation = TRACKUP;
  } else if (_tcscmp(misc, _T("northtrack")) == 0) {
    settings_map.cruise_orientation = TRACKUP;
    settings_map.circling_orientation = TARGETUP;
  }

  ActionInterface::SendSettingsMap(true);
}

/* Event_TerrainToplogy Changes
   0       Show
   1       Topography = ON
   2       Topography = OFF
   3       Terrain = ON
   4       Terrain = OFF
   -1      Toggle through 4 stages (off/off, off/on, on/off, on/on)
   -2      Toggle terrain
   -3      Toggle topography
 */

void
InputEvents::sub_TerrainTopography(int vswitch)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (vswitch == -1) {
    // toggle through 4 possible options
    char val = 0;

    if (settings_map.topography_enabled)
      val++;
    if (settings_map.terrain.enable)
      val += (char)2;

    val++;
    if (val > 3)
      val = 0;

    settings_map.topography_enabled = ((val & 0x01) == 0x01);
    settings_map.terrain.enable = ((val & 0x02) == 0x02);
  } else if (vswitch == -2)
    // toggle terrain
    settings_map.terrain.enable = !settings_map.terrain.enable;
  else if (vswitch == -3)
    // toggle topography
    settings_map.topography_enabled = !settings_map.topography_enabled;
  else if (vswitch == 1)
    // Turn on topography
    settings_map.topography_enabled = true;
  else if (vswitch == 2)
    // Turn off topography
    settings_map.topography_enabled = false;
  else if (vswitch == 3)
    // Turn on terrain
    settings_map.terrain.enable = true;
  else if (vswitch == 4)
    // Turn off terrain
    settings_map.terrain.enable = false;
  else if (vswitch == 0) {
    // Show terrain/topography
    // ARH Let user know what's happening
    TCHAR buf[128];

    if (settings_map.topography_enabled)
      _stprintf(buf, _T("\r\n%s / "), _("On"));
    else
      _stprintf(buf, _T("\r\n%s / "), _("Off"));

    _tcscat(buf, settings_map.terrain.enable
            ? _("On") : _("Off"));

    Message::AddMessage(_("Topography/Terrain"), buf);
    return;
  }

  /* save new values to profile */
  Profile::Set(szProfileDrawTopography,
               settings_map.topography_enabled);
  Profile::Set(szProfileDrawTerrain,
               settings_map.terrain.enable);

  XCSoarInterface::SendSettingsMap(true);
}
