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

// control of calculations, these only changed by user interface
// but are used by calculations

// All of these should be protected by LockFlightData() when setting from outside
// calculation thread; calculation thread should not set these values

extern int    AutoMcMode;
extern bool   EnableCalibration;
extern bool   ForceFinalGlide;
extern bool   AutoForceFinalGlide;
extern int    AutoWindMode;
extern bool   EnableNavBaroAltitude;
extern bool   EnableBlockSTF; // block speed to fly instead of dolphin
extern int    EnableThermalLocator;
extern int    LoggerTimeStepCruise;
extern int    LoggerTimeStepCircling;
extern double SAFETYALTITUDEARRIVAL;
extern double SAFETYALTITUDEBREAKOFF;
extern double SAFETYALTITUDETERRAIN;
extern double SAFTEYSPEED;
extern int    EnableExternalTriggerCruise;

extern short  AverEffTime;
extern double QFEAltitudeOffset; // VENTA3

extern bool   EnableBestAlternate;
extern bool   EnableAlternate1;
extern bool   EnableAlternate2;

// polar info
extern int    BallastSecsToEmpty;
extern bool   BallastTimerActive;
extern double BUGS;
extern double BALLAST;
extern double MACCREADY;
extern bool   AutoMacCready;

extern int TeamCodeRefWaypoint;
extern bool TeammateCodeValid;
extern bool TeamFlarmTracking;
extern TCHAR TeamFlarmCNTarget[4]; // CN of the glider to track

#endif

