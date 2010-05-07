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

#include "GlideComputerAirData.hpp"
#include "WindZigZag.h"
#include "WindAnalyser.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Math/LowPassFilter.hpp"
#include "Math/Earth.hpp"
#include "Math/Constants.h"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Calibration.hpp"
#include "GlideTerrain.hpp"
#include "LocalTime.hpp"
#include "MapWindowProjection.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Atmosphere.h"
#include "LogFile.hpp"
#include "GPSClock.hpp"
#include "ThermalBase.hpp"
#include "TaskClientCalc.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "AirspaceClientCalc.hpp"

#include <algorithm>

using std::min;
using std::max;

#define MinTurnRate  4
#define CruiseClimbSwitch 15
#define ClimbCruiseSwitch 10
#define THERMAL_TIME_MIN 45.0


GlideComputerAirData::GlideComputerAirData(AirspaceClientCalc& airspace,
                                           TaskClientCalc& _task):
  m_airspace(airspace),
  airspace_clock(2.0), // scan airspace every 2 seconds
  ballast_clock(5),  // only update every 5 seconds to stop flooding
		    // the devices
  vario_30s_filter(30),
  netto_30s_filter(30),
  GlideComputerBlackboard(_task)
{
  InitLDRotary(SettingsComputer(), &rotaryLD);

  // JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing, 1);
}


void
GlideComputerAirData::ResetFlight(const bool full)
{
  const AIRCRAFT_STATE as = ToAircraftState(Basic());
  m_airspace.reset_warning(as);

  vario_30s_filter.reset();
  netto_30s_filter.reset();
}

void
GlideComputerAirData::Initialise()
{
  CalibrationInit();
}


/**
 * Calculates some basic values
 */
void
GlideComputerAirData::ProcessBasic()
{
  TerrainHeight();
  ProcessSun();
  SetCalculated().AdjustedAverageThermal = GetAverageThermal();

  if (Basic().TotalEnergyVarioAvailable && !Basic().gps.Replay) {
    CalibrationUpdate(&Basic());
  }
}

/**
 * Calculates some other values
 */
void
GlideComputerAirData::ProcessVertical()
{
  Turning();
  Wind();

  thermallocator.Process(Calculated().Circling,
                         Basic().Time, Basic().Location,
                         Basic().NettoVario,
                         Basic().wind, SetCalculated());

  CuSonde::updateMeasurements(Basic());
  LastThermalStats();
  LD();
  CruiseLD();

  if (!Basic().flight.OnGround && !Calculated().Circling) {
    SetCalculated().AverageLD = CalculateLDRotary(rotaryLD);
  }

  Average30s();
  AverageClimbRate();
  AverageThermal();
  ThermalGain();
}

/**
 * Calculates the wind
 */
void
GlideComputerAirData::Wind()
{
  if (!Basic().flight.Flying || !time_advanced())
    return;

  if (Calculated().TurnMode == CLIMB)
    DoWindCirclingSample();

  // generate new wind vector if altitude changes or a new
  // estimate is available
  DoWindCirclingAltitude();

  // update zigzag wind
  if (((SettingsComputer().AutoWindMode & D_AUTOWIND_ZIGZAG) == D_AUTOWIND_ZIGZAG)
      && !Basic().gps.Replay
      && (Basic().TrueAirspeed > m_task.get_glide_polar().get_Vtakeoff())) {
    double zz_wind_speed;
    double zz_wind_bearing;
    int quality;
    quality = WindZigZagUpdate(Basic(), Calculated(),
			       &zz_wind_speed,
			       &zz_wind_bearing);

    if (quality > 0)
      SetWindEstimate(zz_wind_speed, zz_wind_bearing, quality);
  }
}

/**
 * Passes data to the windanalyser.slot_newFlightMode method
 */
void
GlideComputerAirData::DoWindCirclingMode(const bool left)
{
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING) == D_AUTOWIND_CIRCLING)
    windanalyser.slot_newFlightMode(Basic(), Calculated(), left, 0);
}

/**
 * Passes data to the windanalyser.slot_newSample method
 */
void
GlideComputerAirData::DoWindCirclingSample()
{
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING) == D_AUTOWIND_CIRCLING)
    windanalyser.slot_newSample(Basic(), SetCalculated());
}

/**
 * Passes data to the windanalyser.SlotAltitude method
 */
void
GlideComputerAirData::DoWindCirclingAltitude()
{
  if (SettingsComputer().AutoWindMode > 0)
    windanalyser.slot_Altitude(Basic(), SetCalculated());
}

void
GlideComputerAirData::SetWindEstimate(const double wind_speed,
    const double wind_bearing, const int quality)
{
  Vector v_wind = Vector(SpeedVector(Angle::degrees(fixed(wind_bearing)), 
                                     fixed(wind_speed)));

  windanalyser.slot_newEstimate(Basic(), SetCalculated(), v_wind, quality);
}

void
GlideComputerAirData::AverageClimbRate()
{
  if (Basic().AirspeedAvailable && Basic().TotalEnergyVarioAvailable
      && (!Calculated().Circling)) {

    int vi = iround(Basic().IndicatedAirspeed);

    if ((vi <= 0) || (vi >= SettingsComputer().SafetySpeed)) {
      // out of range
      return;
    }

    if (Basic().acceleration.Available) {
      if (fabs(fabs(Basic().acceleration.Gload) - 1.0) > 0.25) {
        // G factor too high
        return;
      }
    }

    if (Basic().TrueAirspeed > 0) {
      // TODO: Check this is correct for TAS/IAS
      fixed ias_to_tas = Basic().IndicatedAirspeed /
        Basic().TrueAirspeed;
      fixed w_tas = Basic().TotalEnergyVario * ias_to_tas;

      SetCalculated().AverageClimbRate[vi] += w_tas;
      SetCalculated().AverageClimbRateN[vi]++;
    }
  }
}

#ifdef NEWCLIMBAV
ClimbAverageCalculator climbAverageCalculator;
void
GlideComputerAirData::Average30s()
{
  Calculated().Average30s =
    climbAverageCalculator.GetAverage(Basic().Time, Basic().Altitude, 30);
  Calculated().NettoAverage30s = Calculated().Average30s;
}
#else

void
GlideComputerAirData::Average30s()
{
  if (!time_advanced() 
      || (Calculated().Circling != LastCalculated().Circling)) {

    vario_30s_filter.reset();
    netto_30s_filter.reset();
    SetCalculated().Average30s = Basic().TotalEnergyVario;
    SetCalculated().NettoAverage30s = Basic().NettoVario;
  }

  if (!time_advanced()) {
    return;
  }

  const unsigned Elapsed = (unsigned)(Basic().Time -
                                      LastBasic().Time);
  for (unsigned i = 0; i < Elapsed; ++i) {
    if (vario_30s_filter.update(Basic().TotalEnergyVario)) {
      SetCalculated().Average30s =
        LowPassFilter(Calculated().Average30s, vario_30s_filter.average(), 0.8);
    }
    if (netto_30s_filter.update(Basic().NettoVario)) {
      SetCalculated().NettoAverage30s =
        LowPassFilter(Calculated().NettoAverage30s, netto_30s_filter.average(), 0.8);
    }
  }
}
#endif


void
GlideComputerAirData::AverageThermal()
{
  if (Calculated().ClimbStartTime >= 0) {
    if (Basic().Time > Calculated().ClimbStartTime) {
      double Gain = Basic().TEAltitude - Calculated().ClimbStartAlt;
      SetCalculated().AverageThermal =
          Gain / (Basic().Time - Calculated().ClimbStartTime);
      }
  }
}

void
GlideComputerAirData::MaxHeightGain()
{
  if (!Basic().flight.Flying)
    return;

  if (Calculated().MinAltitude > 0) {
    fixed height_gain = Basic().NavAltitude
      - Calculated().MinAltitude;
    SetCalculated().MaxHeightGain = max(height_gain, Calculated().MaxHeightGain);
  } else {
    SetCalculated().MinAltitude = Basic().NavAltitude;
  }

  SetCalculated().MinAltitude = min(Basic().NavAltitude, Calculated().MinAltitude);
}

void
GlideComputerAirData::ThermalGain()
{
  if (Calculated().ClimbStartTime >= 0) {
    if(Basic().Time >= Calculated().ClimbStartTime) {
      SetCalculated().ThermalGain = Basic().TEAltitude - Calculated().ClimbStartAlt;
    }
  }
}

void
GlideComputerAirData::LD()
{
  if (time_retreated()) {
    SetCalculated().LDvario = INVALID_GR;
    SetCalculated().LD = INVALID_GR;
  }

  if (time_advanced()) {
    double DistanceFlown = Distance(Basic().Location, LastBasic().Location);

    SetCalculated().LD =
      UpdateLD(Calculated().LD,
	       DistanceFlown,
	       Basic().NavAltitude - LastBasic().NavAltitude,
               0.1);

    if (!Basic().flight.OnGround && !Calculated().Circling) {
      InsertLDRotary(&rotaryLD,(int)DistanceFlown,
                     (int)Basic().NavAltitude);
    }
  }

  // LD instantaneous from vario, updated every reading..
  if (Basic().TotalEnergyVarioAvailable && Basic().AirspeedAvailable &&
      Basic().flight.Flying) {
    SetCalculated().LDvario = UpdateLD(Calculated().LDvario,
				       Basic().IndicatedAirspeed,
                                       -Basic().TotalEnergyVario,
				       0.3);
  } else {
    SetCalculated().LDvario = INVALID_GR;
  }
}

void
GlideComputerAirData::CruiseLD()
{
  if(!Calculated().Circling) {
    if (Calculated().CruiseStartTime < 0) {
      SetCalculated().CruiseStartLocation = Basic().Location;
      SetCalculated().CruiseStartAlt = Basic().NavAltitude;
      SetCalculated().CruiseStartTime = Basic().Time;
    } else {
      double DistanceFlown = Distance(Basic().Location,
                                      Calculated().CruiseStartLocation);
      SetCalculated().CruiseLD =
          UpdateLD(Calculated().CruiseLD,
                   DistanceFlown,
                   Calculated().CruiseStartAlt - Basic().NavAltitude,
                   0.5);
    }
  }
}



/**
 * Reads the current terrain height
 */
void
GlideComputerAirData::TerrainHeight()
{
  short Alt = TERRAIN_INVALID;

  terrain.Lock();
  if (terrain.GetMap()) {
    // want most accurate rounding here
    RasterRounding rounding(*terrain.GetMap());
    Alt = terrain.GetTerrainHeight(Basic().Location, rounding);
  }
  terrain.Unlock();

  SetCalculated().TerrainValid = (Alt > TERRAIN_INVALID);
  SetCalculated().TerrainAlt = std::max((short)0, Alt);
}


/**
 * 1. Detects time retreat and calls ResetFlight if GPS lost
 * 2. Detects change in replay status and calls ResetFlight if so
 * 3. Calls DetectStartTime and saves the time of flight
 * @return true as default, false if something is wrong in time
 */
bool
GlideComputerAirData::FlightTimes()
{
  if (Basic().gps.Replay != LastBasic().gps.Replay) {
    // reset flight before/after replay logger
    ResetFlight(Basic().gps.Replay);
  }

  if (Basic().Time != 0 && time_retreated()) {
    // 20060519:sgi added (Basic().Time != 0) due to always return here
    // if no GPS time available
    if (!Basic().gps.NAVWarning) {
      // Reset statistics.. (probably due to being in IGC replay mode)
      ResetFlight(false);
    }
    return false;
  }

  TakeoffLanding();

  return true;
}

void
GlideComputerAirData::ProcessIdle()
{
  BallastDump();
  TerrainFootprint(MapProjection().GetScreenDistanceMeters());
  if (airspace_clock.check_advance(Basic().Time)
      && SettingsComputer().EnableAirspaceWarnings) {
    AirspaceWarning();
  }
}

/**
 * Detects takeoff and landing events
 */
void
GlideComputerAirData::TakeoffLanding()
{
  if (Basic().GroundSpeed > fixed_one) {
    // stop system from shutting down if moving
    XCSoarInterface::InterfaceTimeoutReset();
  }

  if (Basic().flight.Flying && !LastBasic().flight.Flying) {
    OnTakeoff();
  } else if (!Basic().flight.Flying && LastBasic().flight.Flying) {
    OnLanding();
  }
}

void
GlideComputerAirData::OnLanding()
{
  // JMWX  restore data calculated at finish so
  // user can review flight as at finish line

  if (Calculated().common_stats.task_finished) {
    RestoreFinish();
  }
}

void
GlideComputerAirData::OnTakeoff()
{
  // reset stats on takeoff
  ResetFlight();

  // save stats in case we never finish
  SaveFinish();
}



void
GlideComputerAirData::AirspaceWarning()
{
  m_airspace.set_flight_levels(Basic().pressure);

  const AIRCRAFT_STATE as = ToAircraftState(Basic());
  if (m_airspace.update_warning(as))
    airspaceWarningEvent.trigger();
}


void
GlideComputerAirData::TerrainFootprint(double screen_range)
{
  // initialise base
  SetCalculated().TerrainBase = Calculated().TerrainAlt;

  // estimate max range (only interested in at most one screen
  // distance away) except we need to scan for terrain base, so 20km
  // search minimum is required

  GlidePolar glide_polar = m_task.get_glide_polar();

  terrain.Lock();
  GlideTerrain g_terrain(SettingsComputer(), terrain);

  g_terrain.set_max_range(fixed(max(20000.0, screen_range)));
  
  const fixed d_bearing = fixed_360 / TERRAIN_ALT_INFO::NUMTERRAINSWEEPS;

  unsigned i=0;

  AIRCRAFT_STATE state = ToAircraftState(Basic());
  for (fixed ang = fixed_zero;
       ang<= fixed_360;
       ang+= d_bearing, i++) {

    state.TrackBearing = Angle::degrees(ang);

    TerrainIntersection its = g_terrain.find_intersection(state, glide_polar);
    
    SetCalculated().GlideFootPrint[i] = its.location;
  } 

  terrain.Unlock();

  SetCalculated().TerrainBase = g_terrain.get_terrain_base();
  SetCalculated().Experimental = Calculated().TerrainBase;
}

void
GlideComputerAirData::BallastDump()
{
  double dt = ballast_clock.delta_advance(Basic().Time);

  if (!SettingsComputer().BallastTimerActive || (dt <= 0)) {
    return;
  }

  GlidePolar glide_polar = m_task.get_glide_polar();
  double BALLAST = glide_polar.get_ballast();
  double percent_per_second =
      1.0 / max(10.0, (double)SettingsComputer().BallastSecsToEmpty);

  BALLAST -= dt * percent_per_second;
  if (BALLAST < 0) {
    /// TODO SettingsComputer().BallastTimerActive = false;
    BALLAST = 0.0;
  }
  glide_polar.set_ballast((fixed)BALLAST);
  m_task.set_glide_polar(glide_polar);
}

void
GlideComputerAirData::OnSwitchClimbMode(bool isclimb, bool left)
{
  InitLDRotary(SettingsComputer(), &rotaryLD);

  // Tell the windanalyser of the new flight mode
  DoWindCirclingMode(left);
}

/**
 * Calculate the circling time percentage and call the thermal band calculation
 * @param Rate Current turn rate
 */
void
GlideComputerAirData::PercentCircling(const double Rate)
{
  // TODO accuracy: TB: this would only work right if called every ONE second!

  // JMW circling % only when really circling,
  // to prevent bad stats due to flap switches and dolphin soaring

  // if (Circling)
  if (Calculated().Circling && (Rate > MinTurnRate)) {
    // Add one second to the circling time
    // timeCircling += (Basic->Time-LastTime);
    SetCalculated().timeCircling += fixed_one;

    // Add the Vario signal to the total climb height
    SetCalculated().TotalHeightClimb += Basic().GPSVario;

    // call ThermalBand function here because it is then explicitly
    // tied to same condition as %circling calculations
    ThermalBand();
  } else {
    // Add one second to the cruise time
    // timeCruising += (Basic->Time-LastTime);
    SetCalculated().timeCruising += fixed_one;
  }

  // Calculate the circling percentage
  if (Calculated().timeCruising + Calculated().timeCircling > 1) {
    SetCalculated().PercentCircling = 100.0 * (Calculated().timeCircling) /
        (Calculated().timeCruising + Calculated().timeCircling);
  } else {
    SetCalculated().PercentCircling = 0.0;
  }
}

/**
 * Calculates the turn rate and the derived features.
 * Determines the current flight mode (cruise/circling).
 */
void
GlideComputerAirData::Turning()
{
  // You can't be circling unless you're flying
  if (!Basic().flight.Flying || !time_advanced())
    return;

  // JMW limit rate to 50 deg per second otherwise a big spike
  // will cause spurious lock on circling for a long time
  double Rate = max(-50.0, min(50.0, (double)Basic().TurnRate));

  // average rate, to detect essing
  // TODO: use rotary buffer
  static double rate_history[60];
  double rate_ave = 0;
  for (int i = 59; i > 0; i--) {
    rate_history[i] = rate_history[i - 1];
    rate_ave += rate_history[i];
  }
  rate_history[0] = Rate;
  rate_ave /= 60;

  // Make the turn rate more smooth using the LowPassFilter
  Rate = LowPassFilter(LastCalculated().SmoothedTurnRate, Rate, 0.3);
  SetCalculated().SmoothedTurnRate = Rate;

  // Determine which direction we are circling
  bool LEFT = false;
  if(Rate < 0) {
    LEFT= true;
    Rate *= -1;
  }

  // Calculate circling time percentage and call thermal band calculation
  PercentCircling(Rate);

  // Force cruise or climb mode if external device says so
  bool forcecruise = false;
  bool forcecircling = false;
  if (SettingsComputer().EnableExternalTriggerCruise && !Basic().gps.Replay) {
    forcecircling = triggerClimbEvent.test();
    forcecruise = !forcecircling;
  }

  switch (Calculated().TurnMode) {
  case CRUISE:
    // If (in cruise mode and beginning of circling detected)
    if ((Rate >= MinTurnRate) || (forcecircling)) {
      // Remember the start values of the turn
      SetCalculated().TurnStartTime = Basic().Time;
      SetCalculated().TurnStartLocation = Basic().Location;
      SetCalculated().TurnStartAltitude = Basic().NavAltitude;
      SetCalculated().TurnStartEnergyHeight = Basic().EnergyHeight;
      SetCalculated().TurnMode = WAITCLIMB;
    }
    if (!forcecircling) {
      break;
    }

  case WAITCLIMB:
    if (forcecruise) {
      SetCalculated().TurnMode = CRUISE;
      break;
    }
    if ((Rate >= MinTurnRate) || (forcecircling)) {
      if (((Basic().Time - Calculated().TurnStartTime) > CruiseClimbSwitch)
          || forcecircling) {
        // yes, we are certain now that we are circling
        SetCalculated().Circling = true;

        // JMW Transition to climb
        SetCalculated().TurnMode = CLIMB;

        // Remember the start values of the climbing period
        SetCalculated().ClimbStartLocation = Calculated().TurnStartLocation;
        SetCalculated().ClimbStartAlt = Calculated().TurnStartAltitude
            + Calculated().TurnStartEnergyHeight;
        SetCalculated().ClimbStartTime = Calculated().TurnStartTime;

        // set altitude for start of circling (as base of climb)
        OnClimbBase(Calculated().TurnStartAltitude);

        // consider code: InputEvents GCE - Move this to InputEvents
        // Consider a way to take the CircleZoom and other logic
        // into InputEvents instead?
        // JMW: NO.  Core functionality must be built into the
        // main program, unable to be overridden.
        OnSwitchClimbMode(true, LEFT);
      }
    } else {
      // nope, not turning, so go back to cruise
      SetCalculated().TurnMode = CRUISE;
    }
    break;

  case CLIMB:
    if ((Rate < MinTurnRate) || (forcecruise)) {
      // Remember the end values of the turn
      SetCalculated().TurnStartTime = Basic().Time;
      SetCalculated().TurnStartLocation = Basic().Location;
      SetCalculated().TurnStartAltitude = Basic().NavAltitude;
      SetCalculated().TurnStartEnergyHeight = Basic().EnergyHeight;

      // JMW Transition to cruise, due to not properly turning
      SetCalculated().TurnMode = WAITCRUISE;
    }
    if (!forcecruise) {
      break;
    }

  case WAITCRUISE:
    if (forcecircling) {
      SetCalculated().TurnMode = CLIMB;
      break;
    }
    if((Rate < MinTurnRate) || forcecruise) {
      if (((Basic().Time - Calculated().TurnStartTime) > ClimbCruiseSwitch)
          || forcecruise) {
        // yes, we are certain now that we are cruising again
        SetCalculated().Circling = false;

        // Transition to cruise
        SetCalculated().TurnMode = CRUISE;
        SetCalculated().CruiseStartLocation = Calculated().TurnStartLocation;
        SetCalculated().CruiseStartAlt = Calculated().TurnStartAltitude;
        SetCalculated().CruiseStartTime = Calculated().TurnStartTime;

        OnClimbCeiling();

        OnSwitchClimbMode(false, LEFT);
      }
    } else {
      // nope, we are circling again
      // JMW Transition back to climb, because we are turning again
      SetCalculated().TurnMode = CLIMB;
    }
    break;
  default:
    // error, go to cruise
    SetCalculated().TurnMode = CRUISE;
  }
}

void
GlideComputerAirData::ThermalSources()
{
  GEOPOINT ground_location;
  fixed ground_altitude;

  EstimateThermalBase(Calculated().ThermalEstimate_Location,
                      Basic().NavAltitude,
                      Calculated().LastThermalAverage,
                      Basic().wind,
                      &ground_location,
                      &ground_altitude);

  if (ground_altitude > 0) {
    double tbest = 0;
    int ibest = 0;

    for (int i = 0; i < MAX_THERMAL_SOURCES; i++) {
      if (Calculated().ThermalSources[i].LiftRate < 0.0) {
        ibest = i;
        break;
      }
      double dt = Basic().Time - Calculated().ThermalSources[i].Time;
      if (dt > tbest) {
        tbest = dt;
        ibest = i;
      }
    }

    SetCalculated().ThermalSources[ibest].LiftRate = Calculated().LastThermalAverage;
    SetCalculated().ThermalSources[ibest].Location = ground_location;
    SetCalculated().ThermalSources[ibest].GroundHeight = ground_altitude;
    SetCalculated().ThermalSources[ibest].Time = Basic().Time;
  }
}

void
GlideComputerAirData::LastThermalStats()
{
  if((Calculated().Circling == false) && (LastCalculated().Circling == true)
     && (Calculated().ClimbStartTime >= 0)) {

    double ThermalTime = Calculated().CruiseStartTime - Calculated().ClimbStartTime;

    if (ThermalTime > 0) {
      double ThermalGain = Calculated().CruiseStartAlt
          + Basic().EnergyHeight - Calculated().ClimbStartAlt;

      if ((ThermalGain > 0) && (ThermalTime > THERMAL_TIME_MIN)) {

        SetCalculated().LastThermalAverage = ThermalGain / ThermalTime;
        SetCalculated().LastThermalGain = ThermalGain;
        SetCalculated().LastThermalTime = ThermalTime;

        OnDepartedThermal();
      }
    }
  }
}

void
GlideComputerAirData::OnDepartedThermal()
{
  ThermalSources();
}

void
GlideComputerAirData::ThermalBand()
{
  if (!time_advanced())
    return;

  // JMW TODO accuracy: Should really work out dt here,
  //           but i'm assuming constant time steps

  const fixed dheight = Basic().working_band_height;

  int index, i, j;

  if (!positive(Basic().working_band_height)) {
    return; // nothing to do.
  }
  if (Calculated().MaxThermalHeight == 0) {
    SetCalculated().MaxThermalHeight = dheight;
  }

  // only do this if in thermal and have been climbing
  if ((!Calculated().Circling) || (Calculated().Average30s < 0))
    return;

  if (dheight > Calculated().MaxThermalHeight) {
    // moved beyond ceiling, so redistribute buckets
    fixed max_thermal_height_new;
    fixed tmpW[NUMTHERMALBUCKETS];
    int tmpN[NUMTHERMALBUCKETS];
    fixed h;

    // calculate new buckets so glider is below max
    fixed hbuk = Calculated().MaxThermalHeight/NUMTHERMALBUCKETS;

    max_thermal_height_new = max(fixed_one, Calculated().MaxThermalHeight);
    while (max_thermal_height_new < dheight) {
      max_thermal_height_new += hbuk;
    }

    // reset counters
    for (i = 0; i < NUMTHERMALBUCKETS; i++) {
      tmpW[i]= 0.0;
      tmpN[i]= 0;
    }
    // shift data into new buckets
    for (i = 0; i < NUMTHERMALBUCKETS; i++) {
      h = (i) * (Calculated().MaxThermalHeight) / (NUMTHERMALBUCKETS);
      // height of center of bucket
      j = iround(NUMTHERMALBUCKETS * h / max_thermal_height_new);

      if (j < NUMTHERMALBUCKETS) {
        if (Calculated().ThermalProfileN[i] > 0) {
          tmpW[j] += Calculated().ThermalProfileW[i];
          tmpN[j] += Calculated().ThermalProfileN[i];
        }
      }
    }

    for (i = 0; i < NUMTHERMALBUCKETS; i++) {
      SetCalculated().ThermalProfileW[i] = tmpW[i];
      SetCalculated().ThermalProfileN[i] = tmpN[i];
    }
    SetCalculated().MaxThermalHeight = max_thermal_height_new;
  }

  index = min(NUMTHERMALBUCKETS - 1,
              iround(NUMTHERMALBUCKETS
                     * (dheight / max(fixed_one, Calculated().MaxThermalHeight))));

  SetCalculated().ThermalProfileW[index] += Basic().TotalEnergyVario;
  SetCalculated().ThermalProfileN[index]++;
}


void
GlideComputerAirData::ProcessSun()
{
  sun.CalcSunTimes(Basic().Location, Basic().DateTime,
                   GetUTCOffset() / 3600);
  SetCalculated().TimeSunset = sun.TimeOfSunSet;
}


GlidePolar 
GlideComputerAirData::get_glide_polar() const
{
  return m_task.get_glide_polar();
}
