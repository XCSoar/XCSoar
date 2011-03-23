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

#include "GlideComputerAirData.hpp"
#include "Wind/WindZigZag.hpp"
#include "Wind/WindAnalyser.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsMap.hpp"
#include "Math/LowPassFilter.hpp"
#include "Math/Earth.hpp"
#include "Math/Constants.h"
#include "Terrain/RasterTerrain.hpp"
#include "LocalTime.hpp"
#include "MapWindowProjection.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "LogFile.hpp"
#include "GPSClock.hpp"
#include "ThermalBase.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Defines.h"
#include "NMEA/Aircraft.hpp"

#include <algorithm>

#define fixed_inv_2g fixed(1.0/(2.0*9.81))

using std::min;
using std::max;

static const fixed MinTurnRate(4);
static const fixed CruiseClimbSwitch(15);
static const fixed ClimbCruiseSwitch(10);
static const fixed THERMAL_TIME_MIN(45);

GlideComputerAirData::GlideComputerAirData(ProtectedAirspaceWarningManager &awm,
                                           ProtectedTaskManager& _task):
  GlideComputerBlackboard(_task),
  m_airspace(awm),
  // scan airspace every second
  airspace_clock(fixed_one),
  // only update every 5 seconds to stop flooding the devices
  ballast_clock(fixed(5)),
  vario_30s_filter(30),
  netto_30s_filter(30)
{
  rotaryLD.init(SettingsComputer());

  // JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing, 1);
}

void
GlideComputerAirData::ResetFlight(const bool full)
{
  const AIRCRAFT_STATE as = ToAircraftState(Basic(), Calculated());
  m_airspace.reset_warning(as);

  vario_30s_filter.reset();
  netto_30s_filter.reset();

  ResetLiftDatabase();

  thermallocator.Reset();

  windanalyser.reset();
}

/**
 * Calculates some basic values
 */
void
GlideComputerAirData::ProcessBasic()
{
  TerrainHeight();
  ProcessSun();
  SetCalculated().ThermalAverageAdjusted = GetAverageThermal();
}

/**
 * Calculates some other values
 */
void
GlideComputerAirData::ProcessVertical()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  TurnRate();
  Turning();
  Wind();
  SelectWind();
  Heading();
  Airspeed();
  BruttoVario();
  NettoVario();

  thermallocator.Process(calculated.Circling,
                         basic.Time, basic.Location,
                         calculated.NettoVario,
                         calculated.wind, calculated.thermal_locator);

  CuSonde::updateMeasurements(basic, calculated);
  LastThermalStats();
  LD();
  CruiseLD();

  if (calculated.flight.Flying && !calculated.Circling)
    calculated.AverageLD = rotaryLD.calculate();

  Average30s();
  AverageClimbRate();
  ThermalGain();
  AverageThermal();
  UpdateLiftDatabase();
}

/**
 * Calculates the wind
 */
void
GlideComputerAirData::Wind()
{
  DERIVED_INFO &calculated = SetCalculated();

  if (!calculated.flight.Flying || !time_advanced())
    return;

  if (SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING) {

    if (calculated.TurnMode == CLIMB)
      windanalyser.slot_newSample(Basic(), calculated);

    // generate new wind vector if altitude changes or a new
    // estimate is available
    windanalyser.slot_Altitude(Basic(), calculated);
  }

  // update zigzag wind
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_ZIGZAG)
      && (Basic().TrueAirspeed > Calculated().glide_polar_task.get_Vtakeoff())) {
    fixed zz_wind_speed;
    Angle zz_wind_bearing;
    int quality;
    quality = WindZigZagUpdate(Basic(), calculated,
                               zz_wind_speed, zz_wind_bearing);

    if (quality > 0)
      SetWindEstimate(zz_wind_speed, zz_wind_bearing, quality);
  }
}

void
GlideComputerAirData::SelectWind()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (basic.ExternalWindAvailable && SettingsComputer().ExternalWind) {
    // external wind available
    calculated.wind = basic.ExternalWind;
    calculated.wind_available = basic.ExternalWindAvailable;

  } else if (SettingsComputer().ManualWindAvailable && SettingsComputer().AutoWindMode == 0) {
    // manual wind only if available and desired
    calculated.wind = SettingsComputer().ManualWind;
    calculated.wind_available.update(basic.Time);

  } else if (Calculated().estimated_wind_available.modified(SettingsComputer().ManualWindAvailable)
             && SettingsComputer().AutoWindMode) {
    // auto wind when available and newer than manual wind
    calculated.wind = Calculated().estimated_wind;
    calculated.wind_available = Calculated().estimated_wind_available;
    XCSoarInterface::SetSettingsComputer().ManualWindAvailable.clear(); // unset manual wind

  } else if (SettingsComputer().ManualWindAvailable
             && SettingsComputer().AutoWindMode) {
    // manual wind overrides auto wind if available
    calculated.wind = SettingsComputer().ManualWind;
    calculated.wind_available = SettingsComputer().ManualWindAvailable;

  } else
   // no wind available
   calculated.wind_available.clear();
}

void
GlideComputerAirData::SetWindEstimate(fixed wind_speed, Angle wind_bearing,
                                      const int quality)
{
  Vector v_wind = Vector(SpeedVector(wind_bearing, wind_speed));

  windanalyser.slot_newEstimate(Basic(), SetCalculated(), v_wind, quality);
}

/**
 * Calculates the heading
 */
void
GlideComputerAirData::Heading()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();
  const SpeedVector wind = calculated.wind;

  if ((positive(basic.GroundSpeed) || wind.is_non_zero()) && Calculated().flight.Flying) {
    fixed x0 = basic.TrackBearing.fastsine() * basic.GroundSpeed;
    fixed y0 = basic.TrackBearing.fastcosine() * basic.GroundSpeed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    calculated.Heading = Angle::radians(atan2(x0, y0)).as_bearing();
  } else {
    calculated.Heading = basic.TrackBearing;
  }
}

void
GlideComputerAirData::Airspeed()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  fixed TrueAirspeedEstimated = fixed_zero;

  if (!basic.AirspeedAvailable) {

    if (!calculated.wind_available) {
      calculated.AirspeedAvailable.clear();
      return;
    }

    const SpeedVector wind = calculated.wind;
    if (positive(basic.GroundSpeed) || wind.is_non_zero()) {
      fixed x0 = basic.TrackBearing.fastsine() * basic.GroundSpeed;
      fixed y0 = basic.TrackBearing.fastcosine() * basic.GroundSpeed;
      x0 += wind.bearing.fastsine() * wind.norm;
      y0 += wind.bearing.fastcosine() * wind.norm;

      TrueAirspeedEstimated = hypot(x0, y0);
    }

    calculated.TrueAirspeed = TrueAirspeedEstimated;
    calculated.IndicatedAirspeed = TrueAirspeedEstimated
      / AtmosphericPressure::AirDensityRatio(basic.GetAltitudeBaroPreferred());
    calculated.AirspeedAvailable.update(basic.Time);

  } else {
    calculated.TrueAirspeed = basic.TrueAirspeed;
    calculated.IndicatedAirspeed = basic.IndicatedAirspeed;
    calculated.AirspeedAvailable = basic.AirspeedAvailable;
  }
}

/**
 * Calculates energy height on TAS basis
 *
 * \f${m/2} \times v^2 = m \times g \times h\f$ therefore \f$h = {v^2}/{2 \times g}\f$
 */
void
GlideComputerAirData::EnergyHeight()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (calculated.AirspeedAvailable)
    calculated.EnergyHeight = sqr(calculated.TrueAirspeed) * fixed_inv_2g;
  else
    /* setting EnergyHeight to zero is the safe approach, as we don't know the kinetic energy
       of the glider for sure. */
    calculated.EnergyHeight = fixed_zero;

  calculated.TEAltitude = basic.NavAltitude + calculated.EnergyHeight;
}

/**
 * 1. Calculates the vario values for gps vario, gps total energy vario and distance vario
 * 2. Sets Vario to GPSVario or received Vario data from instrument
 */
void
GlideComputerAirData::BruttoVario()
{
  const NMEA_INFO &basic = Basic();
  const DERIVED_INFO &calculated = Calculated();
  VARIO_INFO &vario = SetCalculated();

  // Calculate time passed since last calculation
  const fixed dT = basic.Time - LastBasic().Time;

  if (positive(dT)) {
    const fixed Gain = basic.NavAltitude - LastBasic().NavAltitude;
    const fixed GainTE = calculated.TEAltitude - calculated.TEAltitude;

    // estimate value from GPS
    vario.GPSVario = Gain / dT;
    vario.GPSVarioTE = GainTE / dT;
  }

  vario.BruttoVario = basic.TotalEnergyVarioAvailable
    ? basic.TotalEnergyVario
    : vario.GPSVario;
}

void
GlideComputerAirData::NettoVario()
{
  const NMEA_INFO &basic = Basic();
  const DERIVED_INFO &calculated = Calculated();
  VARIO_INFO &vario = SetCalculated();

  vario.GliderSinkRate =
    calculated.flight.Flying && calculated.AirspeedAvailable
    ? - calculated.glide_polar_task.SinkRate(calculated.IndicatedAirspeed,
                                             basic.acceleration.Gload)
    /* the glider sink rate is useless when not flying */
    : fixed_zero;

  vario.NettoVario = basic.NettoVarioAvailable
    ? basic.NettoVario
    : vario.BruttoVario - vario.GliderSinkRate;
}

void
GlideComputerAirData::AverageClimbRate()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (calculated.AirspeedAvailable && basic.TotalEnergyVarioAvailable &&
      !calculated.Circling) {
    int vi = iround(calculated.IndicatedAirspeed);
    if (vi <= 0 || vi >= iround(SettingsComputer().SafetySpeed))
      // out of range
      return;

    if (basic.acceleration.Available)
      if (fabs(fabs(basic.acceleration.Gload) - fixed_one) > fixed(0.25))
        // G factor too high
        return;

    if (positive(calculated.TrueAirspeed)) {
      // TODO: Check this is correct for TAS/IAS
      fixed ias_to_tas = calculated.IndicatedAirspeed / calculated.TrueAirspeed;
      fixed w_tas = basic.TotalEnergyVario * ias_to_tas;

      calculated.AverageClimbRate[vi] += w_tas;
      calculated.AverageClimbRateN[vi]++;
    }
  }
}

void
GlideComputerAirData::Average30s()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (!time_advanced() || calculated.Circling != LastCalculated().Circling) {
    vario_30s_filter.reset();
    netto_30s_filter.reset();
    calculated.Average30s = calculated.BruttoVario;
    calculated.NettoAverage30s = calculated.NettoVario;
  }

  if (!time_advanced())
    return;

  const unsigned Elapsed = (unsigned)(basic.Time - LastBasic().Time);
  for (unsigned i = 0; i < Elapsed; ++i) {
    if (vario_30s_filter.update(calculated.BruttoVario))
      calculated.Average30s =
        LowPassFilter(calculated.Average30s, vario_30s_filter.average(),
                      fixed(0.8));

    if (netto_30s_filter.update(calculated.NettoVario))
      calculated.NettoAverage30s =
        LowPassFilter(calculated.NettoAverage30s, netto_30s_filter.average(),
                      fixed(0.8));
  }
}

void
GlideComputerAirData::AverageThermal()
{
  DERIVED_INFO &calculated = SetCalculated();

  if (positive(calculated.ClimbStartTime) &&
      Basic().Time > calculated.ClimbStartTime)
    calculated.ThermalAverage =
      calculated.ThermalGain / (Basic().Time - calculated.ClimbStartTime);
}

/**
 * This function converts a heading into an unsigned index for the LiftDatabase.
 *
 * This is calculated with Angles to deal with the 360 degree limit.
 *
 * 357 = 0
 * 4 = 0
 * 5 = 1
 * 14 = 1
 * 15 = 2
 * ...
 * @param heading The heading to convert
 * @return The index for the LiftDatabase array
 */
static unsigned
heading_to_index(Angle &heading)
{
  static const Angle afive = Angle::degrees(fixed(5));

  unsigned index =
      floor((heading + afive).as_bearing().value_degrees() / 10);

  return std::max(0u, std::min(35u, index));
}

void
GlideComputerAirData::UpdateLiftDatabase()
{
  DERIVED_INFO &calculated = SetCalculated();

  // Don't update the lift database if we are not in circling mode
  if (!Calculated().Circling)
    return;

  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!LastCalculated().Circling)
    ResetLiftDatabase();

  // Determine the direction in which we are circling
  bool left = negative(Calculated().SmoothedTurnRate);

  // Depending on the direction set the step size sign for the
  // following loop
  Angle heading_step = Angle::degrees(fixed(left ? -10 : 10));

  // Start at the last heading and add heading_step until the current heading
  // is reached. For each heading save the current lift value into the
  // LiftDatabase. Last and current heading are included since they are
  // a part of the ten degree interval most of the time.
  //
  // This is done with Angles to deal with the 360 degrees limit.
  // e.g. last heading 348 degrees, current heading 21 degrees
  //
  // The loop condition stops until the current heading is reached.
  // Depending on the circling direction the current heading will be
  // smaller or bigger then the last one, because of that negative() is
  // tested against the left variable.
  for (Angle h = LastCalculated().Heading;
       left == negative((calculated.Heading - h).as_delta().value_degrees());
       h += heading_step) {
    unsigned index = heading_to_index(h);
    calculated.LiftDatabase[index] = calculated.BruttoVario;
  }

  // detect zero crossing
  if (((calculated.Heading.value_degrees()< fixed_90) && 
       (LastCalculated().Heading.value_degrees()> fixed_270)) ||
      ((LastCalculated().Heading.value_degrees()< fixed_90) && 
       (calculated.Heading.value_degrees()> fixed_270))) {

    fixed h_av = fixed_zero;
    for (unsigned i=0; i<36; ++i) {
      h_av += calculated.LiftDatabase[i];
    }
    h_av/= 36;
    calculated.trace_history.CirclingAverage.push(h_av);
  }
}

void
GlideComputerAirData::ResetLiftDatabase()
{
  DERIVED_INFO &calculated = SetCalculated();

  // Reset LiftDatabase to zero
  for (unsigned i = 0; i < 36; i++)
    calculated.LiftDatabase[i] = fixed_zero;

  calculated.trace_history.CirclingAverage.clear();
}

void
GlideComputerAirData::MaxHeightGain()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (!calculated.flight.Flying)
    return;

  if (positive(Calculated().MinAltitude)) {
    fixed height_gain = basic.NavAltitude - Calculated().MinAltitude;
    calculated.MaxHeightGain = max(height_gain, Calculated().MaxHeightGain);
  } else {
    calculated.MinAltitude = basic.NavAltitude;
  }

  calculated.MinAltitude = min(basic.NavAltitude, Calculated().MinAltitude);
}

void
GlideComputerAirData::ThermalGain()
{
  DERIVED_INFO &calculated = SetCalculated();

  if (positive(Calculated().ClimbStartTime) &&
      Basic().Time >= Calculated().ClimbStartTime)
    calculated.ThermalGain =
      calculated.TEAltitude - Calculated().ClimbStartAlt;
}

void
GlideComputerAirData::LD()
{
  DERIVED_INFO &calculated = SetCalculated();

  if (time_retreated()) {
    calculated.LDvario = fixed(INVALID_GR);
    calculated.LD = fixed(INVALID_GR);
  }

  if (time_advanced()) {
    fixed DistanceFlown = Distance(Basic().Location, LastBasic().Location);

    calculated.LD =
      UpdateLD(Calculated().LD, DistanceFlown,
               LastBasic().NavAltitude - Basic().NavAltitude, fixed(0.1));

    if (calculated.flight.Flying && !Calculated().Circling)
      rotaryLD.add((int)DistanceFlown, (int)Basic().NavAltitude);
  }

  // LD instantaneous from vario, updated every reading..
  if (Basic().TotalEnergyVarioAvailable && Calculated().AirspeedAvailable &&
      calculated.flight.Flying) {
    calculated.LDvario =
      UpdateLD(Calculated().LDvario, Calculated().IndicatedAirspeed,
               -Basic().TotalEnergyVario, fixed(0.3));
  } else {
    calculated.LDvario = fixed(INVALID_GR);
  }
}

void
GlideComputerAirData::CruiseLD()
{
  DERIVED_INFO &calculated = SetCalculated();

  if(!Calculated().Circling) {
    if (negative(Calculated().CruiseStartTime)) {
      calculated.CruiseStartLocation = Basic().Location;
      calculated.CruiseStartAlt = Basic().NavAltitude;
      calculated.CruiseStartTime = Basic().Time;
    } else {
      fixed DistanceFlown = Distance(Basic().Location,
                                     Calculated().CruiseStartLocation);
      calculated.CruiseLD =
          UpdateLD(Calculated().CruiseLD, DistanceFlown,
                   Calculated().CruiseStartAlt - Basic().NavAltitude,
                   fixed_half);
    }
  }
}

/**
 * Reads the current terrain height
 */
void
GlideComputerAirData::TerrainHeight()
{
  const NMEA_INFO &basic = Basic();
  TERRAIN_ALT_INFO &calculated = SetCalculated();

  if (terrain == NULL) {
    calculated.TerrainValid = false;
    calculated.TerrainAlt = fixed_zero;
    calculated.AltitudeAGLValid = false;
    calculated.AltitudeAGL = fixed_zero;
    return;
  }

  short Alt = terrain->GetTerrainHeight(basic.Location);
  if (RasterBuffer::is_special(Alt)) {
    if (RasterBuffer::is_water(Alt))
      /* assume water is 0m MSL; that's the best guess */
      Alt = 0;
    else {
      calculated.TerrainValid = false;
      calculated.TerrainAlt = fixed_zero;
      calculated.AltitudeAGLValid = false;
      calculated.AltitudeAGL = fixed_zero;
      return;
    }
  }

  calculated.TerrainValid = true;
  calculated.TerrainAlt = fixed(Alt);
  calculated.AltitudeAGL = basic.NavAltitude - calculated.TerrainAlt;
  calculated.AltitudeAGLValid = true;
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
  if (Basic().gps.Replay != LastBasic().gps.Replay)
    // reset flight before/after replay logger
    ResetFlight(Basic().gps.Replay);

  if (positive(Basic().Time) && time_retreated()) {
    // 20060519:sgi added (Basic().Time != 0) due to always return here
    // if no GPS time available
    if (Basic().LocationAvailable)
      // Reset statistics.. (probably due to being in IGC replay mode)
      ResetFlight(false);

    return false;
  }

  FlightState(Calculated().glide_polar_task);
  TakeoffLanding();

  return true;
}

void
GlideComputerAirData::ProcessIdle()
{
  BallastDump();
  if (airspace_clock.check_advance(Basic().Time)
      && SettingsComputer().EnableAirspaceWarnings)
    AirspaceWarning();
}

void
GlideComputerAirData::FlightState(const GlidePolar& glide_polar)
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (basic.Time < LastBasic().Time)
    calculated.flight.flying_state_reset();

  // GPS not lost
  if (!basic.LocationAvailable)
    return;

  // Speed too high for being on the ground
  const fixed speed = calculated.AirspeedAvailable
    ? std::max(calculated.TrueAirspeed, basic.GroundSpeed)
    : basic.GroundSpeed;

  if (speed > glide_polar.get_Vtakeoff() ||
      (calculated.AltitudeAGLValid && calculated.AltitudeAGL > fixed(300))) {
    calculated.flight.flying_state_moving(basic.Time);
  } else {
    calculated.flight.flying_state_stationary(basic.Time);
  }
}

/**
 * Detects takeoff and landing events
 */
void
GlideComputerAirData::TakeoffLanding()
{
  if (Calculated().flight.Flying && !LastCalculated().flight.Flying)
    OnTakeoff();
  else if (!Calculated().flight.Flying && LastCalculated().flight.Flying)
    OnLanding();
}

void
GlideComputerAirData::OnLanding()
{
  // JMWX  restore data calculated at finish so
  // user can review flight as at finish line

  if (Calculated().common_stats.task_finished)
    RestoreFinish();
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
  airspace_database.set_flight_levels(Basic().pressure);

  AirspaceActivity day (Calculated().local_date_time.day_of_week);
  airspace_database.set_activity(day);

  const AIRCRAFT_STATE as = ToAircraftState(Basic(), Calculated());
  if (m_airspace.update_warning(as, Calculated().Circling))
    airspaceWarningEvent.trigger();
}

void
GlideComputerAirData::BallastDump()
{
  fixed dt = ballast_clock.delta_advance(Basic().Time);

  if (!SettingsComputer().BallastTimerActive || negative(dt))
    return;

  GlidePolar glide_polar = Calculated().glide_polar_task;
  fixed ballast = glide_polar.get_ballast();
  fixed percent_per_second =
    fixed_one / max(10, SettingsComputer().BallastSecsToEmpty);

  ballast -= dt * percent_per_second;
  if (negative(ballast)) {
    XCSoarInterface::SetSettingsComputer().BallastTimerActive = false;
    ballast = fixed_zero;
  }

  glide_polar.set_ballast(ballast);
  m_task.set_glide_polar(glide_polar);
}

void
GlideComputerAirData::OnSwitchClimbMode(bool isclimb, bool left)
{
  rotaryLD.init(SettingsComputer());

  // Tell the windanalyser of the new flight mode
  if (SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING)
    windanalyser.slot_newFlightMode(Basic(), Calculated(), left, 0);
}

/**
 * Calculate the circling time percentage and call the thermal band calculation
 * @param Rate Current turn rate
 */
void
GlideComputerAirData::PercentCircling(const fixed Rate)
{
  DERIVED_INFO &calculated = SetCalculated();

  WorkingBand();

  // TODO accuracy: TB: this would only work right if called every ONE second!

  // JMW circling % only when really circling,
  // to prevent bad stats due to flap switches and dolphin soaring

  // if (Circling)
  if (Calculated().Circling && (Rate > MinTurnRate)) {
    // Add one second to the circling time
    // timeCircling += (Basic->Time-LastTime);
    calculated.timeCircling += fixed_one;

    // Add the Vario signal to the total climb height
    calculated.TotalHeightClimb += calculated.GPSVario;

    // call ThermalBand function here because it is then explicitly
    // tied to same condition as %circling calculations
    ThermalBand();
  } else {
    // Add one second to the cruise time
    // timeCruising += (Basic->Time-LastTime);
    calculated.timeCruising += fixed_one;
  }

  // Calculate the circling percentage
  if (Calculated().timeCruising + Calculated().timeCircling > fixed_one)
    calculated.PercentCircling = 100 * (Calculated().timeCircling) /
        (Calculated().timeCruising + Calculated().timeCircling);
  else
    calculated.PercentCircling = fixed_zero;
}

/**
 * Calculates the turn rate
 */
void
GlideComputerAirData::TurnRate()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  // Calculate time passed since last calculation
  const fixed dT = basic.Time - LastBasic().Time;

  // Calculate turn rate

  if (!Calculated().flight.Flying) {
    calculated.TurnRate = fixed_zero;
    calculated.TurnRateWind = fixed_zero;
    return;
  }
  if (!positive(dT)) {
    return;
  }

  calculated.TurnRate =
    (basic.TrackBearing - LastBasic().TrackBearing).as_delta().value_degrees() / dT;
  calculated.TurnRateWind =
    (calculated.Heading - LastCalculated().Heading).as_delta().value_degrees() / dT;
}

/**
 * Calculates the turn rate and the derived features.
 * Determines the current flight mode (cruise/circling).
 */
void
GlideComputerAirData::Turning()
{
  DERIVED_INFO &calculated = SetCalculated();

  // You can't be circling unless you're flying
  if (!calculated.flight.Flying || !time_advanced())
    return;

  // JMW limit rate to 50 deg per second otherwise a big spike
  // will cause spurious lock on circling for a long time
  fixed Rate = max(fixed(-50), min(fixed(50), calculated.TurnRate));

  // average rate, to detect essing
  // TODO: use rotary buffer
  static fixed rate_history[60];
  fixed rate_ave = fixed_zero;
  for (int i = 59; i > 0; i--) {
    rate_history[i] = rate_history[i - 1];
    rate_ave += rate_history[i];
  }
  rate_history[0] = Rate;
  rate_ave /= 60;

  // Make the turn rate more smooth using the LowPassFilter
  Rate = LowPassFilter(LastCalculated().SmoothedTurnRate, Rate, fixed(0.3));
  calculated.SmoothedTurnRate = Rate;

  // Determine which direction we are circling
  bool LEFT = false;
  if (negative(Rate)) {
    LEFT = true;
    Rate *= -1;
  }

  // Calculate circling time percentage and call thermal band calculation
  PercentCircling(Rate);

  // Force cruise or climb mode if external device says so
  bool forcecruise = false;
  bool forcecircling = false;
  if (SettingsComputer().EnableExternalTriggerCruise && !Basic().gps.Replay) {
    switch (Basic().SwitchState.FlightMode) {
    case SWITCH_INFO::MODE_UNKNOWN:
      forcecircling = false;
      forcecruise = false;
      break;

    case SWITCH_INFO::MODE_CIRCLING:
      forcecircling = true;
      forcecruise = false;
      break;

    case SWITCH_INFO::MODE_CRUISE:
      forcecircling = false;
      forcecruise = true;
      break;
    }
  }

  switch (Calculated().TurnMode) {
  case CRUISE:
    // If (in cruise mode and beginning of circling detected)
    if ((Rate >= MinTurnRate) || (forcecircling)) {
      // Remember the start values of the turn
      calculated.TurnStartTime = Basic().Time;
      calculated.TurnStartLocation = Basic().Location;
      calculated.TurnStartAltitude = Basic().NavAltitude;
      calculated.TurnStartEnergyHeight = calculated.EnergyHeight;
      calculated.TurnMode = WAITCLIMB;
    }
    if (!forcecircling)
      break;

  case WAITCLIMB:
    if (forcecruise) {
      calculated.TurnMode = CRUISE;
      break;
    }
    if ((Rate >= MinTurnRate) || (forcecircling)) {
      if (((Basic().Time - Calculated().TurnStartTime) > CruiseClimbSwitch)
          || forcecircling) {
        // yes, we are certain now that we are circling
        calculated.Circling = true;

        // JMW Transition to climb
        calculated.TurnMode = CLIMB;

        // Remember the start values of the climbing period
        calculated.ClimbStartLocation = Calculated().TurnStartLocation;
        calculated.ClimbStartAlt = Calculated().TurnStartAltitude
            + Calculated().TurnStartEnergyHeight;
        calculated.ClimbStartTime = Calculated().TurnStartTime;

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
      calculated.TurnMode = CRUISE;
    }
    break;

  case CLIMB:
    if ((Rate < MinTurnRate) || (forcecruise)) {
      // Remember the end values of the turn
      calculated.TurnStartTime = Basic().Time;
      calculated.TurnStartLocation = Basic().Location;
      calculated.TurnStartAltitude = Basic().NavAltitude;
      calculated.TurnStartEnergyHeight = calculated.EnergyHeight;

      // JMW Transition to cruise, due to not properly turning
      calculated.TurnMode = WAITCRUISE;
    }
    if (!forcecruise)
      break;

  case WAITCRUISE:
    if (forcecircling) {
      calculated.TurnMode = CLIMB;
      break;
    }
    if((Rate < MinTurnRate) || forcecruise) {
      if (((Basic().Time - Calculated().TurnStartTime) > ClimbCruiseSwitch)
          || forcecruise) {
        // yes, we are certain now that we are cruising again
        calculated.Circling = false;

        // Transition to cruise
        calculated.TurnMode = CRUISE;
        calculated.CruiseStartLocation = Calculated().TurnStartLocation;
        calculated.CruiseStartAlt = Calculated().TurnStartAltitude;
        calculated.CruiseStartTime = Calculated().TurnStartTime;

        OnClimbCeiling();

        OnSwitchClimbMode(false, LEFT);
      }
    } else {
      // nope, we are circling again
      // JMW Transition back to climb, because we are turning again
      calculated.TurnMode = CLIMB;
    }
    break;

  default:
    // error, go to cruise
    calculated.TurnMode = CRUISE;
  }
}

void
GlideComputerAirData::ThermalSources()
{
  const DERIVED_INFO &calculated = Calculated();
  THERMAL_LOCATOR_INFO &thermal_locator = SetCalculated().thermal_locator;

  if (!thermal_locator.estimate_valid ||
      !positive(calculated.LastThermalAverage))
    return;

  if (calculated.wind.norm / calculated.LastThermalAverage > fixed(10.0)) {
    // thermal strength is so weak compared to wind that source estimate
    // is unlikely to be reliable, so don't calculate or remember it
    return;
  }

  GeoPoint ground_location;
  fixed ground_altitude = fixed_minus_one;
  EstimateThermalBase(thermal_locator.estimate_location,
                      Basic().NavAltitude,
                      calculated.LastThermalAverage,
                      calculated.wind,
                      ground_location,
                      ground_altitude);

  if (positive(ground_altitude)) {
    THERMAL_SOURCE_INFO &source = thermal_locator.AllocateSource(Basic().Time);

    source.LiftRate = calculated.LastThermalAverage;
    source.Location = ground_location;
    source.GroundHeight = ground_altitude;
    source.Time = Basic().Time;
  }
}

void
GlideComputerAirData::LastThermalStats()
{
  DERIVED_INFO &calculated = SetCalculated();

  if (calculated.Circling != false ||
      LastCalculated().Circling != true ||
      !positive(calculated.ClimbStartTime))
    return;

  fixed ThermalTime = calculated.CruiseStartTime - calculated.ClimbStartTime;

  if (!positive(ThermalTime))
    return;

  fixed ThermalGain = calculated.CruiseStartAlt
      + calculated.EnergyHeight - calculated.ClimbStartAlt;

  if (!positive(ThermalGain) || ThermalTime <= THERMAL_TIME_MIN)
    return;

  calculated.LastThermalAverage = ThermalGain / ThermalTime;
  calculated.LastThermalGain = ThermalGain;
  calculated.LastThermalTime = ThermalTime;

  if (calculated.LastThermalAverageSmooth == fixed_zero)
    calculated.LastThermalAverageSmooth =
        calculated.LastThermalAverage;
  else
    calculated.LastThermalAverageSmooth =
        LowPassFilter(calculated.LastThermalAverageSmooth,
                      calculated.LastThermalAverage, fixed(0.3));

  OnDepartedThermal();
}

void
GlideComputerAirData::OnDepartedThermal()
{
  ThermalSources();
}

void
GlideComputerAirData::WorkingBand()
{
  const DERIVED_INFO &calculated = Calculated();
  ThermalBandInfo &tbi = SetCalculated().thermal_band;

  const fixed h_safety = SettingsComputer().route_planner.safety_height_terrain +
    Calculated().TerrainBase;

  tbi.working_band_height = calculated.TEAltitude - h_safety;
  if (negative(tbi.working_band_height)) {
    tbi.working_band_fraction = fixed_zero;
    return;
  }

  const fixed max_height = Calculated().thermal_band.MaxThermalHeight;
  if (positive(max_height))
    tbi.working_band_fraction = tbi.working_band_height / max_height;
  else
    tbi.working_band_fraction = fixed_one;

  tbi.working_band_ceiling = std::max(max_height + h_safety,
                                      calculated.TEAltitude);
}

void
GlideComputerAirData::ThermalBand()
{
  if (!time_advanced())
    return;

  // JMW TODO accuracy: Should really work out dt here,
  //           but i'm assuming constant time steps

  ThermalBandInfo &tbi = SetCalculated().thermal_band;

  const fixed dheight = tbi.working_band_height;

  if (!positive(dheight))
    return; // nothing to do.

  if (tbi.MaxThermalHeight == fixed_zero)
    tbi.MaxThermalHeight = dheight;

  // only do this if in thermal and have been climbing
  if ((!Calculated().Circling) || negative(Calculated().Average30s))
    return;

  tbi.add(dheight, Calculated().BruttoVario);
}

void
GlideComputerAirData::ProcessSun()
{
  SunEphemeris sun;
  sun.CalcSunTimes(Basic().Location, Basic().DateTime,
                   fixed(GetUTCOffset()) / 3600);
  SetCalculated().TimeSunset = fixed(sun.TimeOfSunSet);
  SetCalculated().SunAzimuth = sun.Azimuth;
}
