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

#include "Profile/ComputerProfile.hpp"
#include "Profile/TaskProfile.hpp"
#include "Profile/Profile.hpp"
#include "SettingsComputer.hpp"

namespace Profile {
  static void Load(SETTINGS_WIND &settings);
  static void Load(SETTINGS_LOGGER &settings);
  static void Load(SETTINGS_SOUND &settings);
  static void Load(SETTINGS_TEAMCODE &settings);
  static void Load(SETTINGS_VOICE &settings);
  static void Load(SETTINGS_PLACES_OF_INTEREST &settings);
  static void Load(SETTINGS_FEATURES &settings);
};

void
Profile::Load(SETTINGS_WIND &settings)
{
  Get(szProfileAutoWind, settings.AutoWindMode);
  Get(szProfileExternalWind, settings.ExternalWind);
}

void
Profile::Load(SETTINGS_LOGGER &settings)
{
  Get(szProfileLoggerTimeStepCruise, settings.LoggerTimeStepCruise);
  Get(szProfileLoggerTimeStepCircling, settings.LoggerTimeStepCircling);
  Get(szProfileLoggerShort, settings.LoggerShortName);
  Get(szProfileDisableAutoLogger, settings.DisableAutoLogger);
}

void
Profile::Load(SETTINGS_SOUND &settings)
{
  Get(szProfileSoundAudioVario, settings.EnableSoundVario);
  Get(szProfileSoundTask, settings.EnableSoundTask);
  Get(szProfileSoundModes, settings.EnableSoundModes);
  Get(szProfileSoundVolume, settings.SoundVolume);
  Get(szProfileSoundDeadband, settings.SoundDeadband);
}

void
Profile::Load(SETTINGS_TEAMCODE &settings)
{
  Get(szProfileTeamcodeRefWaypoint, settings.TeamCodeRefWaypoint);
}

void
Profile::Load(SETTINGS_VOICE &settings)
{
  Get(szProfileVoiceClimbRate, settings.EnableVoiceClimbRate);
  Get(szProfileVoiceTerrain, settings.EnableVoiceTerrain);
  Get(szProfileVoiceWaypointDistance, settings.EnableVoiceWaypointDistance);
  Get(szProfileVoiceTaskAltitudeDifference,
      settings.EnableVoiceTaskAltitudeDifference);
  Get(szProfileVoiceMacCready, settings.EnableVoiceMacCready);
  Get(szProfileVoiceNewWaypoint, settings.EnableVoiceNewWaypoint);
  Get(szProfileVoiceInSector, settings.EnableVoiceInSector);
  Get(szProfileVoiceAirspace, settings.EnableVoiceAirspace);
}

void
Profile::Load(SETTINGS_PLACES_OF_INTEREST &settings)
{
  Get(szProfileHomeWaypoint, settings.HomeWaypoint);
  settings.HomeLocationAvailable =
    GetGeoPoint(szProfileHomeLocation, settings.HomeLocation);
}

void
Profile::Load(SETTINGS_FEATURES &settings)
{
  GetEnum(szProfileFinalGlideTerrain, settings.FinalGlideTerrain);
  Get(szProfileBlockSTF, settings.EnableBlockSTF);
  Get(szProfileEnableNavBaroAltitude, settings.EnableNavBaroAltitude);
}

void
Profile::Load(SETTINGS_COMPUTER &settings)
{
  Load((SETTINGS_WIND &)settings);
  Load((SETTINGS_LOGGER &)settings);
  Load((SETTINGS_SOUND &)settings);
  Load((SETTINGS_TEAMCODE &)settings);
  Load((SETTINGS_VOICE &)settings);
  Load((SETTINGS_PLACES_OF_INTEREST &)settings);
  Load((SETTINGS_FEATURES &)settings);

  Get(szProfileEnableExternalTriggerCruise,
      settings.EnableExternalTriggerCruise);

  GetEnum(szProfileAverEffTime, settings.AverEffTime);

  Get(szProfileSetSystemTimeFromGPS, settings.SetSystemTimeFromGPS);
  Get(szProfileUTCOffset, settings.UTCOffset);
  if (settings.UTCOffset > 12 * 3600)
    settings.UTCOffset -= 24 * 3600;

  Load(settings.task);
}
