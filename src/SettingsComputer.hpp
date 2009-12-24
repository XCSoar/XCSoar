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

#include "Task/TaskBehaviour.hpp"

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


struct SETTINGS_WIND {
  /**
   * AutoWind calculation mode
   * 0: Manual
   * 1: Circling
   * 2: ZigZag
   * 3: Both
   */
  int AutoWindMode;
};

struct SETTINGS_LOGGER {
  /** Logger interval in cruise mode */
  int LoggerTimeStepCruise;
  /** Logger interval in circling mode */
  int LoggerTimeStepCircling;
  /** Use short IGC filenames for the logger files */
  bool LoggerShortName;
  bool DisableAutoLogger;
};

struct SETTINGS_POLAR {
  unsigned POLARID;
  /** ManoeuveringSpeed */
  double SafetySpeed;
  // polar info
  int BallastSecsToEmpty;
  bool BallastTimerActive;
  double SafetyMacCready;
  bool AbortSafetyUseCurrent;
  double RiskGamma;
};

struct SETTINGS_OLC {
  unsigned OLCRules;
  unsigned Handicap;
  bool EnableOLC;
};

struct SETTINGS_SOUND {
  // sound stuff not used?
  bool EnableSoundVario;
  bool EnableSoundTask;
  bool EnableSoundModes;
  int SoundVolume;
  int SoundDeadband;
};

struct SETTINGS_TEAMCODE {
  int TeamCodeRefWaypoint;
  bool TeamFlarmTracking;
  /** CN of the glider to track */
  TCHAR TeamFlarmCNTarget[4];

  /** auto-detected, see also in Info.h */
  TCHAR TeammateCode[10];
  bool TeammateCodeValid;
  /** FlarmId of the glider to track */
  int TeamFlarmIdTarget;
};

struct SETTINGS_VOICE {
  // vegavoice stuff
  bool EnableVoiceClimbRate;
  bool EnableVoiceTerrain;
  bool EnableVoiceWaypointDistance;
  bool EnableVoiceTaskAltitudeDifference;
  bool EnableVoiceMacCready;
  bool EnableVoiceNewWaypoint;
  bool EnableVoiceInSector;
  bool EnableVoiceAirspace;
};

struct SETTINGS_PLACES_OF_INTEREST {
  bool EnableBestAlternate;
  bool EnableAlternate1;
  bool EnableAlternate2;

  /** Array index of the first alternate waypoint */
  int Alternate1; // VENTA3
  /** Array index of the second alternate waypoint */
  int Alternate2;
  /** Array index of the home waypoint */
  int HomeWaypoint;
};


struct SETTINGS_HEIGHT_MARGINS {
  double SafetyAltitudeArrival;
  double SafetyAltitudeBreakoff;
  double SafetyAltitudeTerrain;
};

struct SETTINGS_FEATURES {
  /** AutoMacCready feature enable (true/false) */
  bool AutoMacCready;

  /**
   * AutoMacCready calculation mode
   * 0: Final glide only
   * 1: Set to average if in climb mode
   * 2: Average if in climb mode, final glide in final glide mode
   */
  int AutoMacCreadyMode;

  /** Calculate final glide over terrain */
  int FinalGlideTerrain;

  /** block speed to fly instead of dolphin */
  bool EnableBlockSTF;

  /** Navigate by baro altitude instead of GPS altitude */
  bool EnableNavBaroAltitude;

  int EnableThermalLocator;

  /** Display mode in final glide if in final glide prior to last turnpoint */
  bool AutoForceFinalGlide;
};


struct SETTINGS_COMPUTER: 
  public SETTINGS_AIRSPACE,
  public SETTINGS_WIND,
  public SETTINGS_LOGGER,
  public SETTINGS_POLAR,
  public SETTINGS_OLC,
  public SETTINGS_SOUND,
  public SETTINGS_TEAMCODE,
  public SETTINGS_VOICE,
  public SETTINGS_PLACES_OF_INTEREST,
  public SETTINGS_HEIGHT_MARGINS,
  public SETTINGS_FEATURES,
  public TaskBehaviour
{

  bool EnableCalibration;
  int EnableExternalTriggerCruise;

  short AverEffTime;

  /** local time adjustment */
  int UTCOffset;

};

#endif

