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

#ifndef XCSOAR_NMEA_DERIVED_H
#define XCSOAR_NMEA_DERIVED_H

#include "Math/fixed.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Task/TaskStats/TaskStats.hpp"
#include "Task/TaskStats/CommonStats.hpp"

#define NUMTHERMALBUCKETS 10
#define MAX_THERMAL_SOURCES 20

struct THERMAL_SOURCE_INFO
{
  GEOPOINT Location;
  fixed GroundHeight;
  fixed LiftRate;
  fixed Time;
};

typedef enum {
  CRUISE= 0,
  WAITCLIMB,
  CLIMB,
  WAITCRUISE
} CirclingMode_t;


struct VARIO_INFO
{
  /** Average vertical speed based on 30s */
  fixed Average30s;
  /** Average vertical speed of the airmass based on 30s */
  fixed NettoAverage30s;

  /** Instant glide ratio */
  fixed LD;
  /** Glide ratio while in Cruise mode */
  fixed CruiseLD;
  /** Average glide ratio */
  int AverageLD;

  fixed LDvario;
};

struct CLIMB_INFO
{
  /** Average vertical speed in the thermal */
  fixed AverageThermal;
  /** Average vertical speed in the thermal (minimum 0.0) */
  fixed AdjustedAverageThermal;

  /** Altitude gained while in the thermal */
  fixed ThermalGain;

  /** Average vertical speed in the last thermal */
  fixed LastThermalAverage;
  /** Altitude gained while in the last thermal */
  fixed LastThermalGain;
  /** Time spend in the last thermal */
  fixed LastThermalTime;
};


struct CIRCLING_INFO
{

  /** Turn rate after low pass filter */
  fixed SmoothedTurnRate;

  /** StartLocation of the current/last climb */
  GEOPOINT ClimbStartLocation;
  /** StartAltitude of the current/last climb */
  fixed ClimbStartAlt;
  /** StartTime of the current/last climb */
  fixed ClimbStartTime;

  /** StartLocation of the current/last cruise */
  GEOPOINT CruiseStartLocation;
  /** StartAltitude of the current/last cruise */
  fixed CruiseStartAlt;
  /** StartTime of the current/last cruise */
  fixed CruiseStartTime;

  /** Start/End time of the turn (used for flight mode determination) */
  fixed TurnStartTime;
  /** Start/End location of the turn (used for flight mode determination) */
  GEOPOINT TurnStartLocation;
  /** Start/End altitude of the turn (used for flight mode determination) */
  fixed TurnStartAltitude;
  /** Start/End energy height of the turn (used for flight mode determination) */
  fixed TurnStartEnergyHeight;

  /** Current TurnMode (Cruise, Climb or somewhere between) */
  CirclingMode_t TurnMode;

  /** True if in circling mode, False otherwise */
  int    Circling;

  /** Circling/Cruise ratio in percent */
  fixed PercentCircling;

  /** Time spent in cruise mode */
  fixed timeCruising;
  /** Time spent in circling mode */
  fixed timeCircling;

  fixed MinAltitude;
  fixed MaxHeightGain;
  fixed TotalHeightClimb;
};

struct TERRAIN_ALT_INFO
{
  /** Terrain altitude */
  fixed TerrainAlt;
  /** True if terrain is valid, False otherwise */
  bool   TerrainValid;

  GEOPOINT TerrainWarningLocation;

  /** Final glide ground line */
  GEOPOINT GlideFootPrint[NUMTERRAINSWEEPS+1];

  /** Lowest height within glide range */
  fixed TerrainBase;
};

struct THERMAL_BAND_INFO
{
  // JMW thermal band data
  fixed MaxThermalHeight;
  int    ThermalProfileN[NUMTHERMALBUCKETS];
  fixed ThermalProfileW[NUMTHERMALBUCKETS];
};

struct FLYING_INFO
{
  /** True if airborne, False otherwise */
  int    Flying;
  int    TimeOnGround;
  int    TimeInFlight;
  /** Detects when glider is on ground for several seconds */
  bool OnGround;

  /** Time of flight */
  fixed FlightTime;
  /** Time of takeoff */
  fixed TakeOffTime;
};

struct THERMAL_LOCATOR_INFO
{
  GEOPOINT ThermalEstimate_Location;
  fixed ThermalEstimate_W;
  fixed ThermalEstimate_R;

  /** Position and data of the last thermal sources */
  THERMAL_SOURCE_INFO ThermalSources[MAX_THERMAL_SOURCES];
};


struct TEAMCODE_INFO
{
  /** Team code */
  TCHAR OwnTeamCode[10];
  /** Bearing to the chosen team mate */
  fixed TeammateBearing;
  /** Distance to the chosen team mate */
  fixed TeammateRange;
  /** Position of the chosen team mate */
  GEOPOINT TeammateLocation;
  /** Team code of the chosen team mate */
  TCHAR  TeammateCode[10]; // auto-detected, see also in settings computer.h
  bool   TeammateCodeValid;
};

/**
 * A struct that holds all the calculated values derived from the data in the
 * NMEA_INFO struct
 */
struct DERIVED_INFO: 
  public VARIO_INFO,
  public CLIMB_INFO,
  public CIRCLING_INFO,
  public TERRAIN_ALT_INFO,
  public FLYING_INFO,
  public THERMAL_BAND_INFO,
  public THERMAL_LOCATOR_INFO,
  public TEAMCODE_INFO
{
  fixed V_stf; /**< Speed to fly block/dolphin (m/s) */

  /** Wind speed */
  fixed WindSpeed_estimated;
  /** Wind bearing */
  fixed WindBearing_estimated;

  bool FinalGlide; /**< Used for display mode */

  // reflects whether aircraft is in a start/finish/aat/turn sector
  bool IsInSector;
  bool IsInAirspace;
  bool InFinishSector;
  bool InStartSector;
  int StartSectorWaypoint;

  fixed AverageClimbRate[200];
  long AverageClimbRateN[200];

  fixed ZoomDistance;

  fixed TimeSunset;

  // JMW note, new items should go at the bottom of this struct before experimental!
  fixed Experimental;

  TaskStats task_stats;
  CommonStats common_stats;

  unsigned time_process_gps;
  unsigned time_process_idle;
};

#endif

