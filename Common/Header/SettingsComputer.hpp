/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include <tchar.h>
#include "SettingsAirspace.hpp"

// control of calculations, these only changed by user interface
// but are used read-only by calculations

// All of these should be protected by LockFlightData() when setting
// from outside calculation thread; calculation thread should not set
// these values

/** AutoWindMode (not in use) */
typedef enum
{
  /** 0: Manual */
  D_AUTOWIND_NONE = 0,
  /** 1: Circling */
  D_AUTOWIND_CIRCLING,
  /** 2: ZigZag */
  D_AUTOWIND_ZIGZAG
  /** 3: Both */
} AutoWindModeBits_t;

/** Airspace display modes */
typedef enum
{
  ALLON = 0,
  CLIP,
  AUTO,
  ALLBELOW,
  INSIDE,
  ALLOFF
} AirspaceDisplayMode_t;

struct SETTINGS_COMPUTER
{
  /** AutoMcCready feature enable (true/false) */
  bool AutoMacCready;
  int FinalGlideTerrain;

  /**
   * AutoMcCready calculation mode
   * 0: Final glide only
   * 1: Set to average if in climb mode
   * 2: Average if in climb mode, final glide in final glide mode
   */
  int AutoMcMode;
  bool EnableCalibration;
  bool AutoForceFinalGlide;

  /**
   * AutoWind calculation mode
   * 0: Manual
   * 1: Circling
   * 2: ZigZag
   * 3: Both
   */
  int AutoWindMode;
  bool EnableNavBaroAltitude;
  /** block speed to fly instead of dolphin */
  bool EnableBlockSTF;
  int EnableThermalLocator;
  /** Logger interval in cruise mode */
  int LoggerTimeStepCruise;
  /** Logger interval in circling mode */
  int LoggerTimeStepCircling;
  /** Use short IGC filenames for the logger files */
  bool LoggerShortName;
  bool DisableAutoLogger;
  double SafetyAltitudeArrival;
  double SafetyAltitudeBreakoff;
  double SafetyAltitudeTerrain;
  /** ManoeuveringSpeed */
  double SafetySpeed;
  int EnableExternalTriggerCruise;
  short AverEffTime;
  bool EnableBestAlternate;
  bool EnableAlternate1;
  bool EnableAlternate2;
  // polar info
  int BallastSecsToEmpty;
  bool BallastTimerActive;

  int TeamCodeRefWaypoint;
  bool TeamFlarmTracking;
  /** CN of the glider to track */
  TCHAR TeamFlarmCNTarget[4];

  // sound stuff not used?
  bool EnableSoundVario;
  bool EnableSoundTask;
  bool EnableSoundModes;
  int SoundVolume;
  int SoundDeadband;

  /** local time adjustment */
  int UTCOffset;

  unsigned OLCRules;
  unsigned Handicap;
  bool EnableOLC;

  /** auto-detected, see also in Info.h */
  TCHAR TeammateCode[10];
  bool TeammateCodeValid;
  /** FlarmId of the glider to track */
  int TeamFlarmIdTarget;

  /** Array index of the first alternate waypoint */
  int Alternate1; // VENTA3
  /** Array index of the second alternate waypoint */
  int Alternate2;
  /** Array index of the home waypoint */
  int HomeWaypoint;

  // vegavoice stuff
  bool EnableVoiceClimbRate;
  bool EnableVoiceTerrain;
  bool EnableVoiceWaypointDistance;
  bool EnableVoiceTaskAltitudeDifference;
  bool EnableVoiceMacCready;
  bool EnableVoiceNewWaypoint;
  bool EnableVoiceInSector;
  bool EnableVoiceAirspace;

  /** Airspace warnings enabled (true/false) */
  bool EnableAirspaceWarnings;
  /** Warning time before airspace entry */
  unsigned WarningTime;
  unsigned AcknowledgementTime;

  // airspace
  AirspaceDisplayMode_t AltitudeMode;
  unsigned ClipAltitude;
  unsigned AltWarningMargin;
  int iAirspaceMode[AIRSPACECLASSCOUNT];
};

#endif

