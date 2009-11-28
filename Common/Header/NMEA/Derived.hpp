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

#ifndef XCSOAR_NMEA_DERIVED_H
#define XCSOAR_NMEA_DERIVED_H

#include "Screen/shapelib/mapshape.h"
#include "GeoPoint.hpp"

#define NUMTHERMALBUCKETS 10
#define MAX_THERMAL_SOURCES 20

struct THERMAL_SOURCE_INFO
{
  GEOPOINT Location;
  double GroundHeight;
  double LiftRate;
  double Time;
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
  double Vario;
  /** Vertical speed of the airmass */
  double NettoVario;
  /** GPS-based vario */
  double GPSVario;
  /** GPS-based vario including energy height */
  double GPSVarioTE;

  /** Average vertical speed based on 30s */
  double Average30s;
  /** Average vertical speed of the airmass based on 30s */
  double NettoAverage30s;

  /** Instant glide ratio */
  double LD;
  /** Glide ratio while in Cruise mode */
  double CruiseLD;
  /** Average glide ratio */
  int	 AverageLD;

  /** MacCready speed */
  double VMacCready;

  double BestCruiseTrack;

  /** Average vertical speed in the thermal */
  double AverageThermal;
  /** Average vertical speed in the thermal (minimum 0.0) */
  double AdjustedAverageThermal;

  /** Altitude gained while in the thermal */
  double ThermalGain;

  /** Average vertical speed in the last thermal */
  double LastThermalAverage;
  /** Altitude gained while in the last thermal */
  double LastThermalGain;
  /** Time spend in the last thermal */
  double LastThermalTime;

  /** StartLocation of the current/last climb */
  GEOPOINT ClimbStartLocation;
  /** StartAltitude of the current/last climb */
  double ClimbStartAlt;
  /** StartTime of the current/last climb */
  double ClimbStartTime;

  /** StartLocation of the current/last cruise */
  GEOPOINT CruiseStartLocation;
  /** StartAltitude of the current/last cruise */
  double CruiseStartAlt;
  /** StartTime of the current/last cruise */
  double CruiseStartTime;

  /** Start/End time of the turn (used for flight mode determination) */
  double TurnStartTime;
  /** Start/End location of the turn (used for flight mode determination) */
  GEOPOINT TurnStartLocation;
  /** Start/End altitude of the turn (used for flight mode determination) */
  double TurnStartAltitude;
  /** Start/End energy height of the turn (used for flight mode determination) */
  double TurnStartEnergyHeight;

  /** Current TurnMode (Cruise, Climb or somewhere between) */
  CirclingMode_t TurnMode;

  /** Wind speed */
  double WindSpeed;
  /** Wind bearing */
  double WindBearing;

  /** Bearing (not used) */
  double Bearing;
  /** Bearing including wind factor */
  double Heading;

  /** Terrain altitude */
  double TerrainAlt;
  /** True if terrain is valid, False otherwise */
  bool   TerrainValid;

  /** Altitude over terrain */
  double AltitudeAGL;

  /** True if airborne, False otherwise */
  int    Flying;

  /** True if in circling mode, False otherwise */
  int    Circling;
  /** 1 if on final glide, 0 otherwise */
  int    FinalGlide;

  int    TimeOnGround;
  int    TimeInFlight;
  bool   LandableReachable;

  double NextAltitudeRequired;
  double NextAltitudeRequired0; // mc=0
  double NextAltitudeDifference;
  double NextAltitudeDifference0; // difference with mc=0

  double FinalAltitudeRequired;
  double FinalAltitudeDifference;

  /** Remaining distance of the task */
  double TaskDistanceToGo;
  /** Distance that is already flown of the task */
  double TaskDistanceCovered;
  /** Estimated time that is required to complete the task */
  double TaskTimeToGo;
  /** StartTime of the task */
  double TaskStartTime;
  double TaskSpeed;
  double TaskSpeedInstantaneous;
  double TaskAltitudeRequired;
  double TaskAltitudeDifference;
  double TaskAltitudeDifference0; // difference with mc=0
  double TaskAltitudeRequiredFromStart;

  double LDFinish;
  double LDNext;

  double LegDistanceToGo;
  double LegDistanceCovered;
  double LegTimeToGo;
  double LegStartTime;
  double LegSpeed;

  /** Predicted position after airspace warning time */
  GEOPOINT NextLocation;
  /** Predicted altitude after airspace warning time */
  double NextAltitude;
  /** Predicted altitude over terrain after airspace warning time */
  double NextAltitudeAGL;

  double AATMaxDistance;
  double AATMinDistance;
  double AATTargetDistance;
  double AATTimeToGo;
  double AATMaxSpeed;
  double AATTargetSpeed;
  double AATMinSpeed;

  /** Circling/Cruise ratio in percent */
  double PercentCircling;

  GEOPOINT TerrainWarningLocation;

  // JMW moved calculated waypoint info here
  double WaypointBearing;
  double WaypointDistance;
  double WaypointSpeed;

  // JMW thermal band data
  double MaxThermalHeight;
  int    ThermalProfileN[NUMTHERMALBUCKETS];
  double ThermalProfileW[NUMTHERMALBUCKETS];

  /** Optimum speed to fly instantaneously */
  double VOpt;

  /** Estimated track bearing at next time step @author JMW */
  double NextTrackBearing;

  /** Whether Speed-To-Fly audio are valid or not */
  bool STFMode;

  /** Energy height excess to slow to best glide speed @author JMW */
  double EnergyHeight;

  /** Turn rate */
  double TurnRate;
  /** Turn rate after low pass filter */
  double SmoothedTurnRate;

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
  double NavAltitude;

  /** True if task was started valid, False otherwise */
  bool ValidStart;
  /** Airspeed of the moment the task was started */
  double TaskStartSpeed;
  /** Altitude of the moment the task was started */
  double TaskStartAltitude;
  /** True if task was finished valid, False otherwise */
  bool ValidFinish;

  double LDvario;

  GEOPOINT ThermalEstimate_Location;
  double ThermalEstimate_W;
  double ThermalEstimate_R;

  /** Position and data of the last thermal sources */
  THERMAL_SOURCE_INFO ThermalSources[MAX_THERMAL_SOURCES];

  /** Final glide ground line */
  pointObj GlideFootPrint[NUMTERRAINSWEEPS+1];

  /** Team code */
  TCHAR OwnTeamCode[10];
  /** Bearing to the chosen team mate */
  double TeammateBearing;
  /** Distance to the chosen team mate */
  double TeammateRange;
  /** Position of the chosen team mate */
  GEOPOINT TeammateLocation;
  /** Team code of the chosen team mate */
  TCHAR  TeammateCode[10]; // auto-detected, see also in settings computer.h
  bool   TeammateCodeValid;

  /** Time of flight */
  double FlightTime;
  /** Time of takeoff */
  double TakeOffTime;

  double AverageClimbRate[200];
  long AverageClimbRateN[200];

  /** Distance to home airport */
  double HomeDistance;
  /** Direction to home airport */
  double HomeRadial;

  double ZoomDistance;
  double TaskSpeedAchieved;
  double TrueAirspeedEstimated;

  /** Time spent in cruise mode */
  double timeCruising;
  /** Time spent in circling mode */
  double timeCircling;

  double MinAltitude;
  double MaxHeightGain;

  /** Turn rate based on heading (including wind) */
  double TurnRateWind;

  /** Estimated bank angle */
  double BankAngle;
  /** Estimated pitch angle */
  double PitchAngle;

  double MacCreadyRisk;
  double TaskTimeToGoTurningNow;
  double TotalHeightClimb;
  double DistanceVario;
  double GliderSinkRate;

  /** Calculated Gload (assuming balanced turn) */
  double Gload;

  /**
   * Average turn rate over 60 calculations multiplied by 100 and
   * divided by MinTurnRate(=4)
   *
   * not used right now.
   */
  double Essing;

  /** Lowest height within glide range */
  double TerrainBase;
  double TermikLigaPoints;

  /**
   * GRadient to final destination
   *
   * Note: we don't need GRNext since this value is used when going to a landing
   * point, which is always a final glide.
   */
  double GRFinish;

  int    BestAlternate;
  double TimeSunset;

  // JMW note, new items should go at the bottom of this struct before experimental!
  double Experimental;
};

#endif

