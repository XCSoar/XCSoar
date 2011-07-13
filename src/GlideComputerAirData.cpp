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
#include "GlideComputer.hpp"
#include "SettingsComputer.hpp"
#include "Math/LowPassFilter.hpp"
#include "Math/Earth.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "LocalTime.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "ThermalBase.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Defines.h"
#include "NMEA/Aircraft.hpp"
#include "AutoQNH.hpp"
#include "Math/SunEphemeris.hpp"

#include <algorithm>

#define fixed_inv_2g fixed(1.0/(2.0*9.81))

using std::min;
using std::max;

static const fixed MinTurnRate(4);
static const fixed CruiseClimbSwitch(15);
static const fixed ClimbCruiseSwitch(10);
static const fixed THERMAL_TIME_MIN(45);

GlideComputerAirData::GlideComputerAirData(const Waypoints &_way_points,
                                           Airspaces &_airspace_database,
                                           ProtectedAirspaceWarningManager &awm)
  :way_points(_way_points), airspace_database(_airspace_database),
   terrain(NULL),
  m_airspace(awm),
  // scan airspace every second
  airspace_clock(fixed_one)
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
 * these calculations do not require time to have been advanced
 * they should be quick -- may be called from fast vario data
 */
void
GlideComputerAirData::ProcessFast()
{
  EnergyHeight();
  BruttoVario();
  NettoVario();
}

/**
 * Calculates some basic values
 */
void
GlideComputerAirData::ProcessBasic()
{
  TerrainHeight();
  ProcessSun();

  EnergyHeight();
  GPSVario(); // <-- note inserted here, as depends on energy height
  BruttoVario();
  NettoVario();
}

/**
 * Calculates some other values
 */
void
GlideComputerAirData::ProcessVertical()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  AutoQNH::Process(basic, calculated, SettingsComputer(), way_points);

  Heading();
  TurnRate();
  Turning();

  Wind();
  SelectWind();

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
  CurrentThermal();
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
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_ZIGZAG) &&
      Basic().AirspeedAvailable && Basic().AirspeedReal &&
      Basic().TrueAirspeed > SettingsComputer().glide_polar_task.GetVTakeoff()) {
    WindZigZagGlue::Result result = wind_zig_zag.Update(Basic(), calculated);

    if (result.quality > 0)
      SetWindEstimate(result.wind, result.quality);
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
    calculated.wind_available.Update(basic.Time);

  } else if (calculated.estimated_wind_available.Modified(SettingsComputer().ManualWindAvailable)
             && SettingsComputer().AutoWindMode) {
    // auto wind when available and newer than manual wind
    calculated.wind = calculated.estimated_wind;
    calculated.wind_available = calculated.estimated_wind_available;

  } else if (SettingsComputer().ManualWindAvailable
             && SettingsComputer().AutoWindMode) {
    // manual wind overrides auto wind if available
    calculated.wind = SettingsComputer().ManualWind;
    calculated.wind_available = SettingsComputer().ManualWindAvailable;

  } else
   // no wind available
   calculated.wind_available.Clear();
}

void
GlideComputerAirData::SetWindEstimate(const SpeedVector wind,
                                      const int quality)
{
  Vector v_wind = Vector(wind);
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

  if ((positive(basic.GroundSpeed) || wind.is_non_zero()) &&
      calculated.flight.Flying) {
    fixed x0 = basic.track.fastsine() * basic.GroundSpeed;
    fixed y0 = basic.track.fastcosine() * basic.GroundSpeed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    calculated.Heading = Angle::radians(atan2(x0, y0)).as_bearing();
  } else {
    calculated.Heading = basic.track;
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

  if (basic.AirspeedAvailable)
    calculated.EnergyHeight = sqr(basic.TrueAirspeed) * fixed_inv_2g;
  else
    /* setting EnergyHeight to zero is the safe approach, as we don't know the kinetic energy
       of the glider for sure. */
    calculated.EnergyHeight = fixed_zero;

  calculated.TEAltitude = basic.NavAltitude + calculated.EnergyHeight;
}

/**
 * Calculates the vario values for gps vario, gps total energy vario
 * Sets Vario to GPSVario or received Vario data from instrument
 */
void
GlideComputerAirData::GPSVario()
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
}

void
GlideComputerAirData::BruttoVario()
{
  const NMEA_INFO &basic = Basic();
  VARIO_INFO &vario = SetCalculated();

  vario.BruttoVario = basic.TotalEnergyVarioAvailable
    ? basic.TotalEnergyVario
    : vario.GPSVario;
}

void
GlideComputerAirData::NettoVario()
{
  const NMEA_INFO &basic = Basic();
  const DERIVED_INFO &calculated = Calculated();
  const SETTINGS_COMPUTER &settings_computer = SettingsComputer();
  VARIO_INFO &vario = SetCalculated();

  vario.GliderSinkRate =
    calculated.flight.Flying && basic.AirspeedAvailable
    ? - settings_computer.glide_polar_task.SinkRate(basic.IndicatedAirspeed,
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

  if (basic.AirspeedAvailable && positive(basic.IndicatedAirspeed) &&
      positive(basic.TrueAirspeed) &&
      basic.TotalEnergyVarioAvailable &&
      !calculated.Circling &&
      (!basic.acceleration.Available ||
       fabs(fabs(basic.acceleration.Gload) - fixed_one) <= fixed(0.25))) {
    // TODO: Check this is correct for TAS/IAS
    fixed ias_to_tas = basic.IndicatedAirspeed / basic.TrueAirspeed;
    fixed w_tas = basic.TotalEnergyVario * ias_to_tas;

    calculated.climb_history.Add(uround(basic.IndicatedAirspeed), w_tas);
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
  if (Elapsed == 0)
    return;

  for (unsigned i = 0; i < Elapsed; ++i) {
    vario_30s_filter.update(calculated.BruttoVario);
    netto_30s_filter.update(calculated.NettoVario);
  }
  calculated.Average30s = vario_30s_filter.average();
  calculated.NettoAverage30s = netto_30s_filter.average();
}

void
GlideComputerAirData::CurrentThermal()
{
  const DERIVED_INFO &calculated = Calculated();
  OneClimbInfo &current_thermal = SetCalculated().current_thermal;

  if (positive(calculated.ClimbStartTime)) {
    current_thermal.start_time = calculated.ClimbStartTime;
    current_thermal.end_time = Basic().Time;
    current_thermal.gain = calculated.TEAltitude - calculated.ClimbStartAlt;
    current_thermal.CalculateAll();
  } else
    current_thermal.Clear();
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
  if (!calculated.Circling)
    return;

  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!LastCalculated().Circling)
    ResetLiftDatabase();

  // Determine the direction in which we are circling
  bool left = negative(calculated.SmoothedTurnRate);

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

  calculated.ClearLiftDatabase();

  calculated.trace_history.CirclingAverage.clear();
}

void
GlideComputerAirData::MaxHeightGain()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  if (!calculated.flight.Flying)
    return;

  if (positive(calculated.MinAltitude)) {
    fixed height_gain = basic.NavAltitude - calculated.MinAltitude;
    calculated.MaxHeightGain = max(height_gain, calculated.MaxHeightGain);
  } else {
    calculated.MinAltitude = basic.NavAltitude;
  }

  calculated.MinAltitude = min(basic.NavAltitude, calculated.MinAltitude);
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
      UpdateLD(calculated.LD, DistanceFlown,
               LastBasic().NavAltitude - Basic().NavAltitude, fixed(0.1));

    if (calculated.flight.Flying && !calculated.Circling)
      rotaryLD.add((int)DistanceFlown, (int)Basic().NavAltitude);
  }

  // LD instantaneous from vario, updated every reading..
  if (Basic().TotalEnergyVarioAvailable && Basic().AirspeedAvailable &&
      calculated.flight.Flying) {
    calculated.LDvario =
      UpdateLD(calculated.LDvario, Basic().IndicatedAirspeed,
               -Basic().TotalEnergyVario, fixed(0.3));
  } else {
    calculated.LDvario = fixed(INVALID_GR);
  }
}

void
GlideComputerAirData::CruiseLD()
{
  DERIVED_INFO &calculated = SetCalculated();

  if (!calculated.Circling) {
    if (negative(calculated.CruiseStartTime)) {
      calculated.CruiseStartLocation = Basic().Location;
      calculated.CruiseStartAlt = Basic().NavAltitude;
      calculated.CruiseStartTime = Basic().Time;
    } else {
      fixed DistanceFlown = Distance(Basic().Location,
                                     calculated.CruiseStartLocation);
      calculated.CruiseLD =
          UpdateLD(calculated.CruiseLD, DistanceFlown,
                   calculated.CruiseStartAlt - Basic().NavAltitude,
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

  if (!basic.LocationAvailable || terrain == NULL) {
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

  FlightState(SettingsComputer().glide_polar_task);
  TakeoffLanding();

  return true;
}

void
GlideComputerAirData::ProcessIdle()
{
  if (airspace_clock.check_advance(Basic().Time)
      && SettingsComputer().airspace.enable_warnings)
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
  const fixed speed = basic.AirspeedAvailable
    ? std::max(basic.TrueAirspeed, basic.GroundSpeed)
    : basic.GroundSpeed;

  if (speed > glide_polar.GetVTakeoff() ||
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
  airspace_database.set_flight_levels(SettingsComputer().pressure);

  AirspaceActivity day (Calculated().local_date_time.day_of_week);
  airspace_database.set_activity(day);

  const AIRCRAFT_STATE as = ToAircraftState(Basic(), Calculated());
  assert(positive(time_delta()));
  if (m_airspace.update_warning(as, Calculated().Circling, (unsigned)iround(time_delta())))
    SetCalculated().airspace_warnings.latest.Update(Basic().Time);
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
  if (calculated.Circling && (Rate > MinTurnRate)) {
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
  if (calculated.timeCruising + calculated.timeCircling > fixed_one)
    calculated.PercentCircling = 100 * calculated.timeCircling /
        (calculated.timeCruising + calculated.timeCircling);
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

  if (!calculated.flight.Flying) {
    calculated.TurnRate = fixed_zero;
    calculated.TurnRateWind = fixed_zero;
    return;
  }
  if (!positive(dT)) {
    return;
  }

  calculated.TurnRate =
    (basic.track - LastBasic().track).as_delta().value_degrees() / dT;
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

  switch (calculated.TurnMode) {
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
      if (((Basic().Time - calculated.TurnStartTime) > CruiseClimbSwitch)
          || forcecircling) {
        // yes, we are certain now that we are circling
        calculated.Circling = true;

        // JMW Transition to climb
        calculated.TurnMode = CLIMB;

        // Remember the start values of the climbing period
        calculated.ClimbStartLocation = calculated.TurnStartLocation;
        calculated.ClimbStartAlt = calculated.TurnStartAltitude
            + calculated.TurnStartEnergyHeight;
        calculated.ClimbStartTime = calculated.TurnStartTime;

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
      if (((Basic().Time - calculated.TurnStartTime) > ClimbCruiseSwitch)
          || forcecruise) {
        // yes, we are certain now that we are cruising again
        calculated.Circling = false;

        // Transition to cruise
        calculated.TurnMode = CRUISE;
        calculated.CruiseStartLocation = calculated.TurnStartLocation;
        calculated.CruiseStartAlt = calculated.TurnStartAltitude;
        calculated.CruiseStartTime = calculated.TurnStartTime;

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
      !calculated.last_thermal.IsDefined())
    return;

  if (calculated.wind.norm / calculated.last_thermal.lift_rate > fixed(10.0)) {
    // thermal strength is so weak compared to wind that source estimate
    // is unlikely to be reliable, so don't calculate or remember it
    return;
  }

  GeoPoint ground_location;
  fixed ground_altitude = fixed_minus_one;
  EstimateThermalBase(thermal_locator.estimate_location,
                      Basic().NavAltitude,
                      calculated.last_thermal.lift_rate,
                      calculated.wind,
                      ground_location,
                      ground_altitude);

  if (positive(ground_altitude)) {
    THERMAL_SOURCE_INFO &source = thermal_locator.AllocateSource(Basic().Time);

    source.LiftRate = calculated.last_thermal.lift_rate;
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
  if (ThermalTime < THERMAL_TIME_MIN)
    return;

  fixed ThermalGain = calculated.CruiseStartAlt
      + calculated.EnergyHeight - calculated.ClimbStartAlt;
  if (!positive(ThermalGain))
    return;

  calculated.last_thermal.start_time = calculated.ClimbStartTime;
  calculated.last_thermal.end_time = calculated.CruiseStartTime;
  calculated.last_thermal.gain = ThermalGain;
  calculated.last_thermal.duration = ThermalTime;
  calculated.last_thermal.CalculateLiftRate();

  if (calculated.LastThermalAverageSmooth == fixed_zero)
    calculated.LastThermalAverageSmooth =
        calculated.last_thermal.lift_rate;
  else
    calculated.LastThermalAverageSmooth =
        LowPassFilter(calculated.LastThermalAverageSmooth,
                      calculated.last_thermal.lift_rate, fixed(0.3));

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
    calculated.TerrainBase;

  tbi.working_band_height = calculated.TEAltitude - h_safety;
  if (negative(tbi.working_band_height)) {
    tbi.working_band_fraction = fixed_zero;
    return;
  }

  const fixed max_height = calculated.thermal_band.MaxThermalHeight;
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

  tbi.Add(dheight, Calculated().BruttoVario);
}

void
GlideComputerAirData::ProcessSun()
{
  if (!Basic().LocationAvailable)
    return;

  DERIVED_INFO &calculated = SetCalculated();

  SunEphemeris sun;
  sun.CalcSunTimes(Basic().Location, Basic().DateTime,
                   fixed(GetUTCOffset()) / 3600);
  calculated.TimeSunset = fixed(sun.TimeOfSunSet);
  calculated.SunAzimuth = sun.Azimuth;
}
