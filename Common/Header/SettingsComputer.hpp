/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#ifndef XCSOAR_SETTINGS_COMPUTER_HPP
#define XCSOAR_SETTINGS_COMPUTER_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// control of calculations, these only changed by user interface
// but are used read-only by calculations

// All of these should be protected by LockFlightData() when setting
// from outside calculation thread; calculation thread should not set
// these values

// AutoWindMode
#define D_AUTOWIND_CIRCLING 1
#define D_AUTOWIND_ZIGZAG 2
// 0: Manual
// 1: Circling
// 2: ZigZag
// 3: Both

// AutoMcMode
// 0: Final glide only
// 1: Set to average if in climb mode
// 2: Average if in climb mode, final glide in final glide mode


typedef struct _SETTINGS_COMPUTER {
  bool AutoMacCready;
 int    FinalGlideTerrain;
 int    AutoMcMode;
 bool   EnableCalibration;
 bool   AutoForceFinalGlide;
 int    AutoWindMode;
 bool   EnableNavBaroAltitude;
 bool   EnableBlockSTF; // block speed to fly instead of dolphin
 int    EnableThermalLocator;
 int    LoggerTimeStepCruise;
 int    LoggerTimeStepCircling;
 bool   LoggerShortName;
 bool   DisableAutoLogger;
 double SAFETYALTITUDEARRIVAL;
 double SAFETYALTITUDEBREAKOFF;
 double SAFETYALTITUDETERRAIN;
 double SAFTEYSPEED;
 int    EnableExternalTriggerCruise;
 short  AverEffTime;
 bool   EnableBestAlternate;
 bool   EnableAlternate1;
 bool   EnableAlternate2;
  // polar info
 int    BallastSecsToEmpty;
 bool   BallastTimerActive;

 int    TeamCodeRefWaypoint;
 bool   TeamFlarmTracking;
 TCHAR  TeamFlarmCNTarget[4]; // CN of the glider to track
  
  // sound stuff not used?
 bool   EnableSoundVario;
 bool   EnableSoundTask;
 bool   EnableSoundModes;
 int    SoundVolume;
 int    SoundDeadband;
} SETTINGS_COMPUTER;

#endif

