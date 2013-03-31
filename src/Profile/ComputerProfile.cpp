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

#include "Profile/ComputerProfile.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/TaskProfile.hpp"
#include "Profile/TrackingProfile.hpp"
#include "Profile/Profile.hpp"
#include "ContestProfile.hpp"
#include "ComputerSettings.hpp"

namespace Profile {
  static void Load(WindSettings &settings);
  static void Load(PolarSettings &settings);
  static void Load(LoggerSettings &settings);
  static void Load(TeamCodeSettings &settings);
  static void Load(VoiceSettings &settings);
  static void Load(PlacesOfInterestSettings &settings);
  static void Load(FeaturesSettings &settings);
  static void Load(CirclingSettings &settings);
};

void
Profile::Load(WindSettings &settings)
{
  unsigned auto_wind_mode = settings.GetLegacyAutoWindMode();
  if (Get(ProfileKeys::AutoWind, auto_wind_mode))
    settings.SetLegacyAutoWindMode(auto_wind_mode);

  Get(ProfileKeys::ExternalWind, settings.use_external_wind);
}

void
Profile::Load(PolarSettings &settings)
{
  fixed degradation;
  if (Get(ProfileKeys::PolarDegradation, degradation) &&
      degradation >= fixed(0.5) && degradation <= fixed(1))
    settings.SetDegradationFactor(degradation);
}

void
Profile::Load(LoggerSettings &settings)
{
  Get(ProfileKeys::LoggerTimeStepCruise, settings.time_step_cruise);
  Get(ProfileKeys::LoggerTimeStepCircling, settings.time_step_circling);

  if (!GetEnum(ProfileKeys::AutoLogger, settings.auto_logger)) {
    // Legacy
    bool disable_auto_logger;
    if (Get(ProfileKeys::DisableAutoLogger, disable_auto_logger))
      settings.auto_logger =
          disable_auto_logger ? LoggerSettings::AutoLogger::OFF :
                                LoggerSettings::AutoLogger::ON;
  }

  Get(ProfileKeys::LoggerID, settings.logger_id.buffer(),
      settings.logger_id.MAX_SIZE);
  Get(ProfileKeys::PilotName, settings.pilot_name.buffer(),
      settings.pilot_name.MAX_SIZE);
  Get(ProfileKeys::EnableFlightLogger, settings.enable_flight_logger);
  Get(ProfileKeys::EnableNMEALogger, settings.enable_nmea_logger);
}

void
Profile::Load(TeamCodeSettings &settings)
{
  Get(ProfileKeys::TeamcodeRefWaypoint, settings.team_code_reference_waypoint);
}

void
Profile::Load(VoiceSettings &settings)
{
  Get(ProfileKeys::VoiceClimbRate, settings.voice_climb_rate_enabled);
  Get(ProfileKeys::VoiceTerrain, settings.voice_terrain_enabled);
  Get(ProfileKeys::VoiceWaypointDistance, settings.voice_waypoint_distance_enabled);
  Get(ProfileKeys::VoiceTaskAltitudeDifference,
      settings.voice_task_altitude_difference_enabled);
  Get(ProfileKeys::VoiceMacCready, settings.voice_mac_cready_enabled);
  Get(ProfileKeys::VoiceNewWaypoint, settings.voice_new_waypoint_enabled);
  Get(ProfileKeys::VoiceInSector, settings.voice_in_sector_enabled);
  Get(ProfileKeys::VoiceAirspace, settings.voice_airspace_enabled);
}

void
Profile::Load(PlacesOfInterestSettings &settings)
{
  Get(ProfileKeys::HomeWaypoint, settings.home_waypoint);
  settings.home_location_available =
    GetGeoPoint(ProfileKeys::HomeLocation, settings.home_location);
}

void
Profile::Load(FeaturesSettings &settings)
{
  GetEnum(ProfileKeys::FinalGlideTerrain, settings.final_glide_terrain);
  Get(ProfileKeys::BlockSTF, settings.block_stf_enabled);
  Get(ProfileKeys::EnableNavBaroAltitude, settings.nav_baro_altitude_enabled);
}

void
Profile::Load(CirclingSettings &settings)
{
  Get(ProfileKeys::EnableExternalTriggerCruise,
      settings.external_trigger_cruise_enabled);
}

static bool
LoadUTCOffset(int &value_r)
{
  /* NOTE: Until 6.2.4 utc_offset was stored as a positive int in the
     settings file (with negative offsets stored as "utc_offset + 24 *
     3600").  Later versions will create a new signed settings key. */

  int value;
  if (Profile::Get(ProfileKeys::UTCOffsetSigned, value)) {
  } else if (Profile::Get(ProfileKeys::UTCOffset, value)) {
    if (value > 12 * 3600)
      value = (value % (24 * 3600)) - 24 * 3600;
  } else
    /* no profile value present */
    return false;

  if (value > 13 * 3600 || value < -13 * 3600)
    /* illegal value */
    return false;

  value_r = value;
  return true;
}

void
Profile::Load(ComputerSettings &settings)
{
  Load(settings.wind);
  Load(settings.polar);
  Load(settings.team_code);
  Load(settings.voice);
  Load(settings.poi);
  Load(settings.features);
  Load(settings.airspace);
  Load(settings.circling);

  GetEnum(ProfileKeys::AverEffTime, settings.average_eff_time);

  Get(ProfileKeys::SetSystemTimeFromGPS, settings.set_system_time_from_gps);

  LoadUTCOffset(settings.utc_offset);

  Load(settings.task);
  Load(settings.contest);
  Load(settings.logger);

#ifdef HAVE_TRACKING
  Load(settings.tracking);
#endif
}
