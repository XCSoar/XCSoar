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

#include "SettingsComputerBlackboard.hpp"
#include "OS/Clock.hpp"

#include <algorithm>

SettingsComputerBlackboard::SettingsComputerBlackboard()
{
  settings_computer.AutoWindMode= D_AUTOWIND_CIRCLING;
  settings_computer.ExternalWind = true;
  settings_computer.EnableBlockSTF = false;

  settings_computer.TeamCodeRefWaypoint = -1;
  settings_computer.TeamFlarmTracking = false;
  settings_computer.TeamFlarmCNTarget.clear();

  settings_computer.AverEffTime=0;
  settings_computer.SoundVolume = 80;
  settings_computer.SoundDeadband = 5;
  settings_computer.EnableNavBaroAltitude = true;
  settings_computer.EnableExternalTriggerCruise=false;
  settings_computer.LoggerTimeStepCruise=5;
  settings_computer.LoggerTimeStepCircling=1;
  settings_computer.DisableAutoLogger = false;
  settings_computer.LoggerShortName = false;
  settings_computer.UTCOffset = GetSystemUTCOffset();

  // for user-set teammate code
  settings_computer.TeammateCodeValid = false;
  settings_computer.TeamFlarmIdTarget.clear();

  settings_computer.ClearHome();

  settings_computer.EnableVoiceClimbRate = false;
  settings_computer.EnableVoiceTerrain = false;
  settings_computer.EnableVoiceWaypointDistance = false;
  settings_computer.EnableVoiceTaskAltitudeDifference = false;
  settings_computer.EnableVoiceMacCready = false;
  settings_computer.EnableVoiceNewWaypoint = false;
  settings_computer.EnableVoiceInSector = false;
  settings_computer.EnableVoiceAirspace = false;

  settings_computer.airspace.SetDefaults();

  settings_computer.EnableSoundVario = true;
  settings_computer.EnableSoundModes = true;
  settings_computer.EnableSoundTask = true;

  settings_computer.SafetySpeed = fixed(70);
  settings_computer.BallastSecsToEmpty  = 120;
  settings_computer.BallastTimerActive = false;

  settings_computer.enable_olc = true;
}

