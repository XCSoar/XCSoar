/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Dialogs/Error.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Message.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Settings.hpp"
#include "Profile/Current.hpp"
#include "Util/Macros.hpp"
#include "Util/EnumCast.hpp"
#include "Units/Units.hpp"
#include "Protection.hpp"
#include "UtilsSettings.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Audio/VarioGlue.hpp"
#include "OS/Path.hpp"
#include "Util/StringCompare.hxx"

void
InputEvents::eventSounds(const TCHAR *misc)
{
  SoundSettings &settings = CommonInterface::SetUISettings().sound;
 // bool OldEnableSoundVario = EnableSoundVario;

  if (StringIsEqual(misc, _T("toggle")))
    settings.vario.enabled = !settings.vario.enabled;
  else if (StringIsEqual(misc, _T("on")))
    settings.vario.enabled = true;
  else if (StringIsEqual(misc, _T("off")))
    settings.vario.enabled = false;
  else if (StringIsEqual(misc, _T("show"))) {
    if (settings.vario.enabled)
      Message::AddMessage(_("Vario sounds on"));
    else
      Message::AddMessage(_("Vario sounds off"));
    return;
  }

  AudioVarioGlue::Configure(settings.vario);
  Profile::Set(ProfileKeys::SoundAudioVario, settings.vario.enabled);
}

void
InputEvents::eventSnailTrail(const TCHAR *misc)
{
  MapSettings &settings_map = CommonInterface::SetMapSettings();

  if (StringIsEqual(misc, _T("toggle"))) {
    unsigned trail_length = (int)settings_map.trail.length;
    trail_length = (trail_length + 1u) % 4u;
    settings_map.trail.length = (TrailSettings::Length)trail_length;
  } else if (StringIsEqual(misc, _T("off")))
    settings_map.trail.length = TrailSettings::Length::OFF;
  else if (StringIsEqual(misc, _T("long")))
    settings_map.trail.length = TrailSettings::Length::LONG;
  else if (StringIsEqual(misc, _T("short")))
    settings_map.trail.length = TrailSettings::Length::SHORT;
  else if (StringIsEqual(misc, _T("full")))
    settings_map.trail.length = TrailSettings::Length::FULL;
  else if (StringIsEqual(misc, _T("show"))) {
    switch (settings_map.trail.length) {
    case TrailSettings::Length::OFF:
      Message::AddMessage(_("Snail trail off"));
      break;

    case TrailSettings::Length::LONG:
      Message::AddMessage(_("Long snail trail"));
      break;

    case TrailSettings::Length::SHORT:
      Message::AddMessage(_("Short snail trail"));
      break;

    case TrailSettings::Length::FULL:
      Message::AddMessage(_("Full snail trail"));
      break;
    }
  }

  ActionInterface::SendMapSettings(true);
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
  if (StringIsEqual(misc, _T("terrain toggle")))
    sub_TerrainTopography(-2);
  else if (StringIsEqual(misc, _T("topography toggle")))
    sub_TerrainTopography(-3);
  else if (StringIsEqual(misc, _T("topology toggle")))
    sub_TerrainTopography(-3);
  else if (StringIsEqual(misc, _T("terrain on")))
    sub_TerrainTopography(3);
  else if (StringIsEqual(misc, _T("terrain off")))
    sub_TerrainTopography(4);
  else if (StringIsEqual(misc, _T("topography on")))
    sub_TerrainTopography(1);
  else if (StringIsEqual(misc, _T("topography off")))
    sub_TerrainTopography(2);
  else if (StringIsEqual(misc, _T("topology on")))
    sub_TerrainTopography(1);
  else if (StringIsEqual(misc, _T("topology off")))
    sub_TerrainTopography(2);
  else if (StringIsEqual(misc, _T("show")))
    sub_TerrainTopography(0);
  else if (StringIsEqual(misc, _T("toggle")))
    sub_TerrainTopography(-1);

  XCSoarInterface::SendMapSettings(true);
}

// Adjust audio deadband of internal vario sounds
// +: increases deadband
// -: decreases deadband
void
InputEvents::eventAudioDeadband(const TCHAR *misc)
{
  SoundSettings &settings = CommonInterface::SetUISettings().sound;

  if (StringIsEqual(misc, _T("+"))) {
    if (settings.sound_deadband >= 40)
      return;

    ++settings.sound_deadband;
  }
  if (StringIsEqual(misc, _T("-"))) {
    if (settings.sound_deadband <= 0)
      return;

    --settings.sound_deadband;
  }

  Profile::Set(ProfileKeys::SoundDeadband, settings.sound_deadband);

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

  PolarSettings &settings = CommonInterface::SetComputerSettings().polar;
  auto BUGS = settings.bugs;
  auto oldBugs = BUGS;

  if (StringIsEqual(misc, _T("up"))) {
    BUGS += 1 / 10.;
    if (BUGS > 1)
      BUGS = 1;
  } else if (StringIsEqual(misc, _T("down"))) {
    BUGS -= 1 / 10.;
    if (BUGS < 0.5)
      BUGS = 0.5;
  } else if (StringIsEqual(misc, _T("max")))
    BUGS = 1;
  else if (StringIsEqual(misc, _T("min")))
    BUGS = 0.5;
  else if (StringIsEqual(misc, _T("show"))) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%d"), (int)(BUGS * 100));
    Message::AddMessage(_("Bugs performance"), Temp);
  }

  if (BUGS != oldBugs) {
    settings.SetBugs(BUGS);
    protected_task_manager->SetGlidePolar(settings.glide_polar_task);
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

  GlidePolar &polar =
    CommonInterface::SetComputerSettings().polar.glide_polar_task;
  auto BALLAST = polar.GetBallast();
  auto oldBallast = BALLAST;

  if (StringIsEqual(misc, _T("up"))) {
    BALLAST += 1 / 10.;
    if (BALLAST >= 1)
      BALLAST = 1;
  } else if (StringIsEqual(misc, _T("down"))) {
    BALLAST -= 1 / 10.;
    if (BALLAST < 0)
      BALLAST = 0;
  } else if (StringIsEqual(misc, _T("max")))
    BALLAST = 1;
  else if (StringIsEqual(misc, _T("min")))
    BALLAST = 0;
  else if (StringIsEqual(misc, _T("show"))) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%d"), (int)(BALLAST * 100));
    /* xgettext:no-c-format */
    Message::AddMessage(_("Ballast %"), Temp);
  }

  if (BALLAST != oldBallast) {
    polar.SetBallast(BALLAST);
    protected_task_manager->SetGlidePolar(polar);
  }
}

// ProfileLoad
// Loads the profile of the specified filename
void
InputEvents::eventProfileLoad(const TCHAR *misc)
{
  if (!StringIsEmpty(misc)) {
    Profile::LoadFile(Path(misc));

    MapFileChanged = true;
    WaypointFileChanged = true;
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;

    // assuming all is ok, we can...
    Profile::Use(Profile::map);
  }
}

// ProfileSave
// Saves the profile to the specified filename
void
InputEvents::eventProfileSave(const TCHAR *misc)
{
  if (!StringIsEmpty(misc)) {
      try {
        Profile::SaveFile(Path(misc));
      } catch (const std::runtime_error &e) {
        ShowError(e, _("Failed to save file."));
        return;
      }
  }
}

// AdjustForecastTemperature
// Adjusts the maximum ground temperature used by the convection forecast
// +: increases temperature by one degree celsius
// -: decreases temperature by one degree celsius
// show: Shows a status message with the current forecast temperature
void
InputEvents::eventAdjustForecastTemperature(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("+")))
    CommonInterface::SetComputerSettings().forecast_temperature += Temperature::FromKelvin(1);
  else if (StringIsEqual(misc, _T("-")))
    CommonInterface::SetComputerSettings().forecast_temperature -= Temperature::FromKelvin(1);
  else if (StringIsEqual(misc, _T("show"))) {
    auto temperature =
      CommonInterface::GetComputerSettings().forecast_temperature;
    TCHAR Temp[100];
    _stprintf(Temp, _T("%f"), temperature.ToUser());
    Message::AddMessage(_("Forecast temperature"), Temp);
  }
}

void
InputEvents::eventDeclutterLabels(const TCHAR *misc)
{
  static const TCHAR *const msg[] = {
    N_("All"),
    N_("Task & Landables"),
    N_("Task"),
    N_("None"),
    N_("Task & Airfields"),
  };
  static constexpr unsigned int n = ARRAY_SIZE(msg);

  static const TCHAR *const actions[n] = {
    _T("all"),
    _T("task+landables"),
    _T("task"),
    _T("none")
    _T("task+airfields"),
  };

  WaypointRendererSettings::LabelSelection &wls =
    CommonInterface::SetMapSettings().waypoint.label_selection;
  if (StringIsEqual(misc, _T("toggle"))) {
    wls = WaypointRendererSettings::LabelSelection(((unsigned)wls + 1) %  n);
    Profile::Set(ProfileKeys::WaypointLabelSelection, (int)wls);
  } else if (StringIsEqual(misc, _T("show"))) {
    TCHAR tbuf[64];
    _stprintf(tbuf, _T("%s: %s"), _("Waypoint labels"),
              gettext(msg[(unsigned)wls]));
    Message::AddMessage(tbuf);
  }
  else {
    for (unsigned int i=0; i<n; i++)
      if (StringIsEqual(misc, actions[i]))
        wls = (WaypointRendererSettings::LabelSelection)i;
  }

  /* save new values to profile */
  Profile::Set(ProfileKeys::WaypointLabelSelection,
               EnumCast<WaypointRendererSettings::LabelSelection>()(wls));

  ActionInterface::SendMapSettings(true);
}

void
InputEvents::eventAirspaceDisplayMode(const TCHAR *misc)
{
  AirspaceRendererSettings &settings =
    CommonInterface::SetMapSettings().airspace;

  if (StringIsEqual(misc, _T("all")))
    settings.altitude_mode = AirspaceDisplayMode::ALLON;
  else if (StringIsEqual(misc, _T("clip")))
    settings.altitude_mode = AirspaceDisplayMode::CLIP;
  else if (StringIsEqual(misc, _T("auto")))
    settings.altitude_mode = AirspaceDisplayMode::AUTO;
  else if (StringIsEqual(misc, _T("below")))
    settings.altitude_mode = AirspaceDisplayMode::ALLBELOW;
  else if (StringIsEqual(misc, _T("off")))
    settings.altitude_mode = AirspaceDisplayMode::ALLOFF;

  TriggerMapUpdate();
}

void
InputEvents::eventOrientation(const TCHAR *misc)
{
  MapSettings &settings_map = CommonInterface::SetMapSettings();

  if (StringIsEqual(misc, _T("northup"))) {
    settings_map.cruise_orientation = MapOrientation::NORTH_UP;
    settings_map.circling_orientation = MapOrientation::NORTH_UP;
  } else if (StringIsEqual(misc, _T("northcircle"))) {
    settings_map.cruise_orientation = MapOrientation::TRACK_UP;
    settings_map.circling_orientation = MapOrientation::NORTH_UP;
  } else if (StringIsEqual(misc, _T("trackcircle"))) {
    settings_map.cruise_orientation = MapOrientation::NORTH_UP;
    settings_map.circling_orientation = MapOrientation::TRACK_UP;
  } else if (StringIsEqual(misc, _T("trackup"))) {
    settings_map.cruise_orientation = MapOrientation::TRACK_UP;
    settings_map.circling_orientation = MapOrientation::TRACK_UP;
  } else if (StringIsEqual(misc, _T("northtrack"))) {
    settings_map.cruise_orientation = MapOrientation::TRACK_UP;
    settings_map.circling_orientation = MapOrientation::TARGET_UP;
  } else if (StringIsEqual(misc, _T("targetup"))) {
    settings_map.cruise_orientation = MapOrientation::TARGET_UP;
    settings_map.circling_orientation = MapOrientation::TARGET_UP;
  }

  ActionInterface::SendMapSettings(true);
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
  MapSettings &settings_map = CommonInterface::SetMapSettings();

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
  Profile::Set(ProfileKeys::DrawTopography,
               settings_map.topography_enabled);
  Profile::Set(ProfileKeys::DrawTerrain,
               settings_map.terrain.enable);

  XCSoarInterface::SendMapSettings(true);
}
