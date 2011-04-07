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

#ifndef XCSOAR_SETTINGS_COMPUTER_HPP
#define XCSOAR_SETTINGS_COMPUTER_HPP

#include "FLARM/FlarmId.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Atmosphere/Pressure.hpp"

#include <tchar.h>

#include "Task/TaskBehaviour.hpp"
#include "Engine/Navigation/SpeedVector.hpp"
#include "NMEA/Validity.hpp"

#include "SettingsAirspace.hpp"
#include "TeamCodeCalculation.h"

class Waypoint;

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

/**
 * Wind calculator settings
 */
struct SETTINGS_WIND {
/**
 * AutoWind calculation mode
 * 0: Manual
 * 1: Circling
 * 2: ZigZag
 * 3: Both
 */
  int AutoWindMode; 

  /**
   * If enabled, then the wind vector received from external devices
   * overrides XCSoar's internal wind calculation.
   */
  bool ExternalWind;

  /**
   * This is the manual wind set by the pilot. Validity is set when
   * changeing manual wind but does not expire.
   */
  SpeedVector ManualWind;
  Validity ManualWindAvailable;
};

/**
 * Logger settings
 */
struct SETTINGS_LOGGER {
  /** Logger interval in cruise mode */
  int LoggerTimeStepCruise;
  /** Logger interval in circling mode */
  int LoggerTimeStepCircling;
  /** Use short IGC filenames for the logger files */
  bool LoggerShortName;
  bool DisableAutoLogger;
};

/**
 * Glide polar settings
 */
struct SETTINGS_POLAR {
  fixed SafetySpeed;           /**< Manoeuvering speed (m/s) */
  int BallastSecsToEmpty;       /**< Time to drain full ballast (s) */
  bool BallastTimerActive;      /**< Whether the ballast countdown timer is active */
};

struct SETTINGS_SOUND {
  // sound stuff not used?
  bool EnableSoundVario;
  bool EnableSoundTask;
  bool EnableSoundModes;
  int SoundVolume;
  int SoundDeadband;
};

/** 
 * Settings for teamcode calculations
 */
struct SETTINGS_TEAMCODE {
  int TeamCodeRefWaypoint;      /**< Reference waypoint id for code origin */
  bool TeamFlarmTracking;       /**< Whether to enable tracking by FLARM */

  TCHAR TeamFlarmCNTarget[4];   /**< CN of the glider to track */
  TeamCode TeammateCode;       /**< auto-detected, see also in Info.h */

  bool TeammateCodeValid;       /**< Whether the teammate code is valid */  
  FlarmId TeamFlarmIdTarget; /**< FlarmId of the glider to track */
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

/**
 * Options for tracking places of interest as alternates
 */
struct SETTINGS_PLACES_OF_INTEREST {
  /** Array index of the home waypoint */
  int HomeWaypoint;

  bool HomeLocationAvailable;

  GeoPoint HomeLocation;

  void ClearHome();
  void SetHome(const Waypoint &wp);
};


/**
 * Options for glide computer features
 */
struct SETTINGS_FEATURES {
  /** Calculate final glide over terrain */
  enum FinalGlideTerrain {
    FGT_OFF,
    FGT_LINE,
    FGT_SHADE,
  } FinalGlideTerrain;

  /** block speed to fly instead of dolphin */
  bool EnableBlockSTF;

  /** Navigate by baro altitude instead of GPS altitude */
  bool EnableNavBaroAltitude;

  /** Use Gestures */
  bool EnableGestures;
};


struct SETTINGS_COMPUTER: 
  public SETTINGS_AIRSPACE,
  public SETTINGS_WIND,
  public SETTINGS_LOGGER,
  public SETTINGS_POLAR,
  public SETTINGS_SOUND,
  public SETTINGS_TEAMCODE,
  public SETTINGS_VOICE,
  public SETTINGS_PLACES_OF_INTEREST,
  public SETTINGS_FEATURES,
  public TaskBehaviour
{
  int EnableExternalTriggerCruise;

  short AverEffTime;

  /** local time adjustment */
  int UTCOffset;

  /**
   * Troposhere atmosphere model for QNH correction
   */
  AtmosphericPressure pressure;
  Validity pressure_available;

  /** Glide polar used for task calculations */
  GlidePolar glide_polar_task;
};

#endif

