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

#include "Profile/ComputerProfile.hpp"
#include "Profile/TaskProfile.hpp"
#include "Profile/TrackingProfile.hpp"
#include "Profile/Profile.hpp"
#include "ComputerSettings.hpp"

namespace Profile {
  static void Load(WindSettings &settings);
  static void Load(PolarSettings &settings);
  static void Load(LoggerSettings &settings);
  static void Load(SoundSettings &settings);
  static void Load(TeamCodeSettings &settings);
  static void Load(VoiceSettings &settings);
  static void Load(PlacesOfInterestSettings &settings);
  static void Load(FeaturesSettings &settings);
};

void
Profile::Load(WindSettings &settings)
{
  unsigned auto_wind_mode = settings.GetLegacyAutoWindMode();
  if (Get(szProfileAutoWind, auto_wind_mode))
    settings.SetLegacyAutoWindMode(auto_wind_mode);

  Get(szProfileExternalWind, settings.use_external_wind);
}

void
Profile::Load(PolarSettings &settings)
{
  fixed degradation;
  if (Get(ProfilePolarDegradation, degradation) &&
      degradation >= fixed_half && degradation <= fixed_one)
    settings.SetDegradation(degradation);
}

void
Profile::Load(LoggerSettings &settings)
{
  Get(szProfileLoggerTimeStepCruise, settings.time_step_cruise);
  Get(szProfileLoggerTimeStepCircling, settings.time_step_circling);
  Get(szProfileLoggerShort, settings.short_name);

  if (!GetEnum(szProfileAutoLogger, settings.auto_logger)) {
    // Legacy
    bool disable_auto_logger;
    if (Get(szProfileDisableAutoLogger, disable_auto_logger))
      settings.auto_logger =
          disable_auto_logger ? LoggerSettings::AutoLogger::OFF :
                                LoggerSettings::AutoLogger::ON;
  }

  Get(szProfileLoggerID, settings.logger_id.buffer(),
      settings.logger_id.MAX_SIZE);
  Get(szProfilePilotName, settings.pilot_name.buffer(),
      settings.pilot_name.MAX_SIZE);
}

void
Profile::Load(SoundSettings &settings)
{
  Get(szProfileSoundAudioVario, settings.sound_vario_enabled);
  Get(szProfileSoundTask, settings.sound_task_enabled);
  Get(szProfileSoundModes, settings.sound_modes_enabled);
  Get(szProfileSoundVolume, settings.sound_volume);
  Get(szProfileSoundDeadband, settings.sound_deadband);
}

void
Profile::Load(TeamCodeSettings &settings)
{
  Get(szProfileTeamcodeRefWaypoint, settings.team_code_reference_waypoint);
}

void
Profile::Load(VoiceSettings &settings)
{
  Get(szProfileVoiceClimbRate, settings.voice_climb_rate_enabled);
  Get(szProfileVoiceTerrain, settings.voice_terrain_enabled);
  Get(szProfileVoiceWaypointDistance, settings.voice_waypoint_distance_enabled);
  Get(szProfileVoiceTaskAltitudeDifference,
      settings.voice_task_altitude_difference_enabled);
  Get(szProfileVoiceMacCready, settings.voice_mac_cready_enabled);
  Get(szProfileVoiceNewWaypoint, settings.voice_new_waypoint_enabled);
  Get(szProfileVoiceInSector, settings.voice_in_sector_enabled);
  Get(szProfileVoiceAirspace, settings.voice_airspace_enabled);
}

void
Profile::Load(PlacesOfInterestSettings &settings)
{
  Get(szProfileHomeWaypoint, settings.home_waypoint);
  settings.home_location_available =
    GetGeoPoint(szProfileHomeLocation, settings.home_location);
}

void
Profile::Load(FeaturesSettings &settings)
{
  GetEnum(szProfileFinalGlideTerrain, settings.final_glide_terrain);
  Get(szProfileBlockSTF, settings.block_stf_enabled);
  Get(szProfileEnableNavBaroAltitude, settings.nav_baro_altitude_enabled);
}

void
Profile::Load(ComputerSettings &settings)
{
  Load(settings.wind);
  Load(settings.polar);
  Load(settings.sound);
  Load(settings.team_code);
  Load(settings.voice);
  Load(settings.poi);
  Load(settings.features);

  Get(szProfileEnableExternalTriggerCruise,
      settings.external_trigger_cruise_enabled);

  GetEnum(szProfileAverEffTime, settings.average_eff_time);

  Get(szProfileSetSystemTimeFromGPS, settings.set_system_time_from_gps);

  // NOTE: Until 6.2.4 utc_offset was stored as a positive int in the
  // settings file (with negative offsets stored as "utc_offset + 24 * 3600").
  // Later versions will create a new signed settings key.
  if (!Get(szProfileUTCOffsetSigned, settings.utc_offset)) {
    if (Get(szProfileUTCOffset, settings.utc_offset)) {
      if (settings.utc_offset > 12 * 3600)
        settings.utc_offset = (settings.utc_offset % (24 * 3600)) - 24 * 3600;
    }
  }

  if (settings.utc_offset > 13 * 3600 || settings.utc_offset < -13 * 3600)
    settings.utc_offset = 0;

  Load(settings.task);
  Load(settings.logger);

#ifdef HAVE_TRACKING
  Load(settings.tracking);
#endif
}
