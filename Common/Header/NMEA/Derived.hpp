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

/**
 * A struct that holds all the calculated values derived from the data in the
 * NMEA_INFO struct
 */
struct DERIVED_INFO
{
  /** Vertical speed */
  fixed Vario;
  /** Vertical speed of the airmass */
  fixed NettoVario;
  /** GPS-based vario */
  fixed GPSVario;
  /** GPS-based vario including energy height */
  fixed GPSVarioTE;

  /** Average vertical speed based on 30s */
  fixed Average30s;
  /** Average vertical speed of the airmass based on 30s */
  fixed NettoAverage30s;

  /** Instant glide ratio */
  fixed LD;
  /** Glide ratio while in Cruise mode */
  fixed CruiseLD;
  /** Average glide ratio */
  int	 AverageLD;

  /** MacCready speed */
  fixed VMacCready;

  fixed BestCruiseTrack;

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

  /** Wind speed */
  fixed WindSpeed;
  /** Wind bearing */
  fixed WindBearing;

  /** Bearing (not used) */
  fixed Bearing;
  /** Bearing including wind factor */
  fixed Heading;

  /** Terrain altitude */
  fixed TerrainAlt;
  /** True if terrain is valid, False otherwise */
  bool   TerrainValid;

  /** Altitude over terrain */
  fixed AltitudeAGL;

  /** True if airborne, False otherwise */
  int    Flying;

  /** True if in circling mode, False otherwise */
  int    Circling;
  /** 1 if on final glide, 0 otherwise */
  int    FinalGlide;

  int    TimeOnGround;
  int    TimeInFlight;
  bool   LandableReachable;

  fixed NextAltitudeRequired;
  fixed NextAltitudeRequired0; // mc=0
  fixed NextAltitudeDifference;
  fixed NextAltitudeDifference0; // difference with mc=0

  fixed FinalAltitudeRequired;
  fixed FinalAltitudeDifference;

  /** Remaining distance of the task */
  fixed TaskDistanceToGo;
  /** Distance that is already flown of the task */
  fixed TaskDistanceCovered;
  /** Estimated time that is required to complete the task */
  fixed TaskTimeToGo;
  /** StartTime of the task */
  fixed TaskStartTime;
  fixed TaskSpeed;
  fixed TaskSpeedInstantaneous;
  fixed TaskAltitudeRequired;
  fixed TaskAltitudeDifference;
  fixed TaskAltitudeDifference0; // difference with mc=0
  fixed TaskAltitudeRequiredFromStart;

  fixed LDFinish;
  fixed LDNext;

  fixed LegDistanceToGo;
  fixed LegDistanceCovered;
  fixed LegTimeToGo;
  fixed LegStartTime;
  fixed LegSpeed;

  /** Predicted position after airspace warning time */
  GEOPOINT NextLocation;
  /** Predicted altitude after airspace warning time */
  fixed NextAltitude;
  /** Predicted altitude over terrain after airspace warning time */
  fixed NextAltitudeAGL;

  fixed AATMaxDistance;
  fixed AATMinDistance;
  fixed AATTargetDistance;
  fixed AATTimeToGo;
  fixed AATMaxSpeed;
  fixed AATTargetSpeed;
  fixed AATMinSpeed;

  /** Circling/Cruise ratio in percent */
  fixed PercentCircling;

  GEOPOINT TerrainWarningLocation;

  // JMW moved calculated waypoint info here
  fixed WaypointBearing;
  fixed WaypointDistance;
  fixed WaypointSpeed;

  // JMW thermal band data
  fixed MaxThermalHeight;
  int    ThermalProfileN[NUMTHERMALBUCKETS];
  fixed ThermalProfileW[NUMTHERMALBUCKETS];

  /** Optimum speed to fly instantaneously */
  fixed VOpt;

  /** Estimated track bearing at next time step @author JMW */
  fixed NextTrackBearing;

  /** Whether Speed-To-Fly audio are valid or not */
  bool STFMode;

  /** Energy height excess to slow to best glide speed @author JMW */
  fixed EnergyHeight;

  /** Turn rate */
  fixed TurnRate;
  /** Turn rate after low pass filter */
  fixed SmoothedTurnRate;

  // reflects whether aircraft is in a start/finish/aat/turn sector
  bool IsInSector;
  bool IsInAirspace;
  bool InFinishSector;
  bool InStartSector;
  int StartSectorWaypoint;

  unsigned ActiveTaskPoint;
  int ReadyWayPoint;

  /** Detects when glider is on ground for several seconds */
  bool OnGround;

  /** Altitude used for navigation (GPS or Baro) */
  fixed NavAltitude;

  /** True if task was started valid, False otherwise */
  bool ValidStart;
  /** Airspeed of the moment the task was started */
  fixed TaskStartSpeed;
  /** Altitude of the moment the task was started */
  fixed TaskStartAltitude;
  /** True if task was finished valid, False otherwise */
  bool ValidFinish;

  fixed LDvario;

  GEOPOINT ThermalEstimate_Location;
  fixed ThermalEstimate_W;
  fixed ThermalEstimate_R;

  /** Position and data of the last thermal sources */
  THERMAL_SOURCE_INFO ThermalSources[MAX_THERMAL_SOURCES];

  /** Final glide ground line */
  GEOPOINT GlideFootPrint[NUMTERRAINSWEEPS+1];

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

  /** Time of flight */
  fixed FlightTime;
  /** Time of takeoff */
  fixed TakeOffTime;

  fixed AverageClimbRate[200];
  long AverageClimbRateN[200];

  /** Distance to home airport */
  fixed HomeDistance;
  /** Direction to home airport */
  fixed HomeRadial;

  fixed ZoomDistance;
  fixed TaskSpeedAchieved;
  fixed TrueAirspeedEstimated;

  /** Time spent in cruise mode */
  fixed timeCruising;
  /** Time spent in circling mode */
  fixed timeCircling;

  fixed MinAltitude;
  fixed MaxHeightGain;

  /** Turn rate based on heading (including wind) */
  fixed TurnRateWind;

  /** Estimated bank angle */
  fixed BankAngle;
  /** Estimated pitch angle */
  fixed PitchAngle;

  fixed MacCreadyRisk;
  fixed TaskTimeToGoTurningNow;
  fixed TotalHeightClimb;
  fixed DistanceVario;
  fixed GliderSinkRate;

  /** Calculated Gload (assuming balanced turn) */
  fixed Gload;

  /**
   * Average turn rate over 60 calculations multiplied by 100 and
   * divided by MinTurnRate(=4)
   *
   * not used right now.
   */
  fixed Essing;

  /** Lowest height within glide range */
  fixed TerrainBase;
  fixed TermikLigaPoints;

  /**
   * GRadient to final destination
   *
   * Note: we don't need GRNext since this value is used when going to a landing
   * point, which is always a final glide.
   */
  fixed GRFinish;

  int    BestAlternate;
  fixed TimeSunset;

  // JMW note, new items should go at the bottom of this struct before experimental!
  fixed Experimental;
};

#endif

