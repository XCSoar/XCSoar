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

#include "SettingsComputer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "OS/Clock.hpp"
#include "Asset.hpp"

void
SETTINGS_WIND::SetDefaults()
{
  AutoWindMode = D_AUTOWIND_CIRCLING;
  ExternalWind = true;
  ManualWindAvailable.Clear();
}

void
SETTINGS_LOGGER::SetDefaults()
{
  LoggerTimeStepCruise = 5;
  LoggerTimeStepCircling = 1;
  LoggerShortName = false;
  DisableAutoLogger = false;
}

void
SETTINGS_POLAR::SetDefaults()
{
  BallastTimerActive = false;
}

void
SETTINGS_SOUND::SetDefaults()
{
  EnableSoundVario = true;
  EnableSoundTask = true;
  EnableSoundModes = true;
  SoundVolume = 80;
  SoundDeadband = 5;
}

void
SETTINGS_TEAMCODE::SetDefaults()
{
  TeamCodeRefWaypoint = -1;
  TeamFlarmTracking = false;
  TeammateCodeValid = false;
  TeamFlarmCNTarget.clear();
  TeamFlarmIdTarget.Clear();
}

void
SETTINGS_VOICE::SetDefaults()
{
  EnableVoiceClimbRate = false;
  EnableVoiceTerrain = false;
  EnableVoiceWaypointDistance = false;
  EnableVoiceTaskAltitudeDifference = false;
  EnableVoiceMacCready = false;
  EnableVoiceNewWaypoint = false;
  EnableVoiceInSector = false;
  EnableVoiceAirspace = false;
}

void
SETTINGS_PLACES_OF_INTEREST::ClearHome()
{
  HomeWaypoint = -1;
  HomeLocationAvailable = false;
}

void
SETTINGS_PLACES_OF_INTEREST::SetHome(const Waypoint &wp)
{
  HomeWaypoint = wp.id;
  HomeLocation = wp.location;
  HomeLocationAvailable = true;
}

void
SETTINGS_FEATURES::SetDefaults()
{
  FinalGlideTerrain = FGT_LINE;
  EnableBlockSTF = false;
  EnableNavBaroAltitude = true;
}

void
SETTINGS_COMPUTER::SetDefaults()
{
  SETTINGS_WIND::SetDefaults();
  SETTINGS_LOGGER::SetDefaults();
  SETTINGS_POLAR::SetDefaults();
  SETTINGS_SOUND::SetDefaults();
  SETTINGS_TEAMCODE::SetDefaults();
  SETTINGS_VOICE::SetDefaults();
  SETTINGS_PLACES_OF_INTEREST::SetDefaults();
  SETTINGS_FEATURES::SetDefaults();
  SETTINGS_PLACES_OF_INTEREST::SetDefaults();

  EnableExternalTriggerCruise =false;
  AverEffTime = ae30seconds;
  SetSystemTimeFromGPS = is_altair() && is_embedded();
  UTCOffset = GetSystemUTCOffset();
  pressure.SetStandardPressure();
  pressure_available.Clear();
  airspace.SetDefaults();
  task.SetDefaults();
}
