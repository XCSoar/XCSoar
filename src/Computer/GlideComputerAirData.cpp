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
  // JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing, 1);
}

void
GlideComputerAirData::ResetFlight(const bool full)
{
  const AircraftState as = ToAircraftState(Basic(), Calculated());
  m_airspace.reset_warning(as);

  vario_30s_filter.reset();
  netto_30s_filter.reset();

  ResetLiftDatabase();

  thermallocator.Reset();

  windanalyser.reset();
  wind_zig_zag.reset();
  wind_store.reset();
  rotaryLD.init(SettingsComputer());
}

/**
 * Calculates some basic values
 */
void
GlideComputerAirData::ProcessBasic()
{
  TerrainHeight();
  ProcessSun();

  NettoVario();
}

/**
 * Calculates some other values
 */
void
GlideComputerAirData::ProcessVertical()
{
  const NMEAInfo &basic = Basic();
  DerivedInfo &calculated = SetCalculated();

  AutoQNH::Process(basic, calculated, SettingsComputer(), way_points);

  Heading();
  TurnRate();
  Turning();

  Wind();
  SelectWind();

  thermallocator.Process(calculated.circling,
                         basic.time, basic.location,
                         basic.netto_vario,
                         calculated.wind, calculated.thermal_locator);

  CuSonde::updateMeasurements(basic, calculated);
  LastThermalStats();
  LD();
  CruiseLD();

  if (calculated.flight.flying && !calculated.circling)
    calculated.average_ld = rotaryLD.calculate();

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
  DerivedInfo &calculated = SetCalculated();

  if (!calculated.flight.flying || !time_advanced())
    return;

  if (SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING) {

    if (calculated.turn_mode == CLIMB) {
      WindAnalyser::Result result =
        windanalyser.NewSample(Basic());
      if (result.IsValid())
        wind_store.SlotMeasurement(Basic(),
                                   result.wind, result.quality);
    }
  }

  // update zigzag wind
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_ZIGZAG) &&
      Basic().airspeed_available && Basic().airspeed_real &&
      Basic().true_airspeed > SettingsComputer().glide_polar_task.GetVTakeoff()) {
    WindZigZagGlue::Result result = wind_zig_zag.Update(Basic(), calculated);

    if (result.quality > 0)
      SetWindEstimate(result.wind, result.quality);
  }

  if (SettingsComputer().AutoWindMode)
    wind_store.SlotAltitude(Basic(), calculated);
}

void
GlideComputerAirData::SelectWind()
{
  const NMEAInfo &basic = Basic();
  DerivedInfo &calculated = SetCalculated();

  if (basic.external_wind_available && SettingsComputer().ExternalWind) {
    // external wind available
    calculated.wind = basic.external_wind;
    calculated.wind_available = basic.external_wind_available;

  } else if (SettingsComputer().ManualWindAvailable && SettingsComputer().AutoWindMode == 0) {
    // manual wind only if available and desired
    calculated.wind = SettingsComputer().ManualWind;
    calculated.wind_available.Update(basic.clock);

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
  wind_store.SlotMeasurement(Basic(), v_wind, quality);
}

/**
 * Calculates the heading
 */
void
GlideComputerAirData::Heading()
{
  const NMEAInfo &basic = Basic();
  DerivedInfo &calculated = SetCalculated();
  const SpeedVector wind = calculated.wind;

  if (calculated.wind_available &&
      (positive(basic.ground_speed) || wind.is_non_zero()) &&
      calculated.flight.flying) {
    fixed x0 = basic.track.fastsine() * basic.ground_speed;
    fixed y0 = basic.track.fastcosine() * basic.ground_speed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    calculated.heading = Angle::radians(atan2(x0, y0)).as_bearing();
  } else {
    calculated.heading = basic.track;
  }
}

void
GlideComputerAirData::NettoVario()
{
  const MoreData &basic = Basic();
  const DerivedInfo &calculated = Calculated();
  const SETTINGS_COMPUTER &settings_computer = SettingsComputer();
  VarioInfo &vario = SetCalculated();

  vario.sink_rate =
    calculated.flight.flying && basic.airspeed_available
    ? - settings_computer.glide_polar_task.SinkRate(basic.indicated_airspeed,
                                                    basic.acceleration.g_load)
    /* the glider sink rate is useless when not flying */
    : fixed_zero;
}

void
GlideComputerAirData::AverageClimbRate()
{
  const NMEAInfo &basic = Basic();
  DerivedInfo &calculated = SetCalculated();

  if (basic.airspeed_available && positive(basic.indicated_airspeed) &&
      positive(basic.true_airspeed) &&
      basic.total_energy_vario_available &&
      !calculated.circling &&
      (!basic.acceleration.available ||
       fabs(fabs(basic.acceleration.g_load) - fixed_one) <= fixed(0.25))) {
    // TODO: Check this is correct for TAS/IAS
    fixed ias_to_tas = basic.indicated_airspeed / basic.true_airspeed;
    fixed w_tas = basic.total_energy_vario * ias_to_tas;

    calculated.climb_history.Add(uround(basic.indicated_airspeed), w_tas);
  }
}

void
GlideComputerAirData::Average30s()
{
  const MoreData &basic = Basic();
  DerivedInfo &calculated = SetCalculated();

  if (!time_advanced() || calculated.circling != LastCalculated().circling) {
    vario_30s_filter.reset();
    netto_30s_filter.reset();
    calculated.average = basic.BruttoVario;
    calculated.netto_average = basic.netto_vario;
  }

  if (!time_advanced())
    return;

  const unsigned Elapsed = (unsigned)time_delta();
  if (Elapsed == 0)
    return;

  for (unsigned i = 0; i < Elapsed; ++i) {
    vario_30s_filter.update(basic.BruttoVario);
    netto_30s_filter.update(basic.netto_vario);
  }
  calculated.average = vario_30s_filter.average();
  calculated.netto_average = netto_30s_filter.average();
}

void
GlideComputerAirData::CurrentThermal()
{
  const DerivedInfo &calculated = Calculated();
  OneClimbInfo &current_thermal = SetCalculated().current_thermal;

  if (positive(calculated.climb_start_time)) {
    current_thermal.start_time = calculated.climb_start_time;
    current_thermal.end_time = Basic().time;
    current_thermal.gain = Basic().TEAltitude - calculated.climb_start_altitude;
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
  DerivedInfo &calculated = SetCalculated();

  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!calculated.circling && LastCalculated().circling)
    ResetLiftDatabase();

  // Determine the direction in which we are circling
  bool left = negative(calculated.turn_rate_smoothed);

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
  for (Angle h = LastCalculated().heading;
       left == negative((calculated.heading - h).as_delta().value_degrees());
       h += heading_step) {
    unsigned index = heading_to_index(h);
    calculated.lift_database[index] = Basic().BruttoVario;
  }

  // detect zero crossing
  if (((calculated.heading.value_degrees()< fixed_90) && 
       (LastCalculated().heading.value_degrees()> fixed_270)) ||
      ((LastCalculated().heading.value_degrees()< fixed_90) && 
       (calculated.heading.value_degrees()> fixed_270))) {

    fixed h_av = fixed_zero;
    for (unsigned i=0; i<36; ++i) {
      h_av += calculated.lift_database[i];
    }
    h_av/= 36;
    calculated.trace_history.CirclingAverage.push(h_av);
  }
}

void
GlideComputerAirData::ResetLiftDatabase()
{
  DerivedInfo &calculated = SetCalculated();

  calculated.ClearLiftDatabase();

  calculated.trace_history.CirclingAverage.clear();
}

void
GlideComputerAirData::MaxHeightGain()
{
  const MoreData &basic = Basic();
  DerivedInfo &calculated = SetCalculated();

  if (!calculated.flight.flying)
    return;

  if (positive(calculated.min_altitude)) {
    fixed height_gain = basic.NavAltitude - calculated.min_altitude;
    calculated.max_height_gain = max(height_gain, calculated.max_height_gain);
  } else {
    calculated.min_altitude = basic.NavAltitude;
  }

  calculated.min_altitude = min(basic.NavAltitude, calculated.min_altitude);
}

void
GlideComputerAirData::LD()
{
  DerivedInfo &calculated = SetCalculated();

  if (time_retreated()) {
    calculated.ld_vario = fixed(INVALID_GR);
    calculated.ld = fixed(INVALID_GR);
  }

  if (time_advanced()) {
    fixed DistanceFlown = Distance(Basic().location, LastBasic().location);

    calculated.ld =
      UpdateLD(calculated.ld, DistanceFlown,
               LastBasic().NavAltitude - Basic().NavAltitude, fixed(0.1));

    if (calculated.flight.flying && !calculated.circling)
      rotaryLD.add((int)DistanceFlown, (int)Basic().NavAltitude);
  }

  // LD instantaneous from vario, updated every reading..
  if (Basic().total_energy_vario_available && Basic().airspeed_available &&
      calculated.flight.flying) {
    calculated.ld_vario =
      UpdateLD(calculated.ld_vario, Basic().indicated_airspeed,
               -Basic().total_energy_vario, fixed(0.3));
  } else {
    calculated.ld_vario = fixed(INVALID_GR);
  }
}

void
GlideComputerAirData::CruiseLD()
{
  DerivedInfo &calculated = SetCalculated();

  if (!calculated.circling) {
    if (negative(calculated.cruise_start_time)) {
      calculated.cruise_start_location = Basic().location;
      calculated.cruise_start_altitude = Basic().NavAltitude;
      calculated.cruise_start_time = Basic().time;
    } else {
      fixed DistanceFlown = Distance(Basic().location,
                                     calculated.cruise_start_location);
      calculated.cruise_ld =
          UpdateLD(calculated.cruise_ld, DistanceFlown,
                   calculated.cruise_start_altitude - Basic().NavAltitude,
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
  const MoreData &basic = Basic();
  TerrainInfo &calculated = SetCalculated();

  if (!basic.location_available || terrain == NULL) {
    calculated.terrain_valid = false;
    calculated.terrain_altitude = fixed_zero;
    calculated.altitude_agl_valid = false;
    calculated.altitude_agl = fixed_zero;
    return;
  }

  short Alt = terrain->GetTerrainHeight(basic.location);
  if (RasterBuffer::is_special(Alt)) {
    if (RasterBuffer::is_water(Alt))
      /* assume water is 0m MSL; that's the best guess */
      Alt = 0;
    else {
      calculated.terrain_valid = false;
      calculated.terrain_altitude = fixed_zero;
      calculated.altitude_agl_valid = false;
      calculated.altitude_agl = fixed_zero;
      return;
    }
  }

  calculated.terrain_valid = true;
  calculated.terrain_altitude = fixed(Alt);
  calculated.altitude_agl = basic.NavAltitude - calculated.terrain_altitude;
  calculated.altitude_agl_valid = true;
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
  if (Basic().gps.replay != LastBasic().gps.replay)
    // reset flight before/after replay logger
    ResetFlight(Basic().gps.replay);

  if (Basic().time_available && time_retreated()) {
    // 20060519:sgi added (Basic().Time != 0) due to always return here
    // if no GPS time available
    if (Basic().location_available)
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
  if (airspace_clock.check_advance(Basic().time)
      && SettingsComputer().airspace.enable_warnings)
    AirspaceWarning();
}

void
GlideComputerAirData::FlightState(const GlidePolar& glide_polar)
{
  const NMEAInfo &basic = Basic();
  DerivedInfo &calculated = SetCalculated();

  if (time_retreated())
    calculated.flight.Reset();

  // GPS not lost
  if (!basic.location_available)
    return;

  // Speed too high for being on the ground
  const fixed speed = basic.airspeed_available
    ? std::max(basic.true_airspeed, basic.ground_speed)
    : basic.ground_speed;

  if (speed > glide_polar.GetVTakeoff() ||
      (calculated.altitude_agl_valid && calculated.altitude_agl > fixed(300))) {
    calculated.flight.Moving(basic.time);
  } else {
    calculated.flight.Stationary(basic.time);
  }
}

/**
 * Detects takeoff and landing events
 */
void
GlideComputerAirData::TakeoffLanding()
{
  if (Calculated().flight.flying && !LastCalculated().flight.flying)
    OnTakeoff();
  else if (!Calculated().flight.flying && LastCalculated().flight.flying)
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
  if (!time_advanced())
    return;

  airspace_database.set_flight_levels(SettingsComputer().pressure);

  AirspaceActivity day (Calculated().date_time_local.day_of_week);
  airspace_database.set_activity(day);

  const AircraftState as = ToAircraftState(Basic(), Calculated());
  if (m_airspace.update_warning(as, Calculated().circling, (unsigned)iround(time_delta())))
    SetCalculated().airspace_warnings.latest.Update(Basic().clock);
}

void
GlideComputerAirData::OnSwitchClimbMode(bool isclimb, bool left)
{
  rotaryLD.init(SettingsComputer());

  // Tell the windanalyser of the new flight mode
  if (SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING)
    windanalyser.slot_newFlightMode(Calculated(), left, 0);
}

/**
 * Calculate the circling time percentage and call the thermal band calculation
 * @param Rate Current turn rate
 */
void
GlideComputerAirData::PercentCircling(const fixed Rate)
{
  DerivedInfo &calculated = SetCalculated();

  WorkingBand();

  // TODO accuracy: TB: this would only work right if called every ONE second!

  // JMW circling % only when really circling,
  // to prevent bad stats due to flap switches and dolphin soaring

  // if (Circling)
  if (calculated.circling && (Rate > MinTurnRate)) {
    // Add one second to the circling time
    // timeCircling += (Basic->Time-LastTime);
    calculated.time_climb += fixed_one;

    // Add the Vario signal to the total climb height
    calculated.total_height_gain += Basic().GPSVario;

    // call ThermalBand function here because it is then explicitly
    // tied to same condition as %circling calculations
    ThermalBand();
  } else {
    // Add one second to the cruise time
    // timeCruising += (Basic->Time-LastTime);
    calculated.time_cruise += fixed_one;
  }

  // Calculate the circling percentage
  if (calculated.time_cruise + calculated.time_climb > fixed_one)
    calculated.circling_percentage = 100 * calculated.time_climb /
        (calculated.time_cruise + calculated.time_climb);
  else
    calculated.circling_percentage = fixed_zero;
}

/**
 * Calculates the turn rate
 */
void
GlideComputerAirData::TurnRate()
{
  circling_computer.TurnRate(SetCalculated(),
                             Basic(), LastBasic(),
                             Calculated(), LastCalculated());
}

/**
 * Calculates the turn rate and the derived features.
 * Determines the current flight mode (cruise/circling).
 */
void
GlideComputerAirData::Turning()
{
  circling_computer.Turning(SetCalculated(),
                            Basic(), LastBasic(),
                            Calculated(), LastCalculated(),
                            SettingsComputer());

  fixed Rate = Calculated().turn_rate_smoothed;

  if (LastCalculated().turn_mode == WAITCLIMB &&
      Calculated().turn_mode == CLIMB)
    OnSwitchClimbMode(true, Rate);
  else if (LastCalculated().turn_mode == WAITCRUISE &&
           Calculated().turn_mode == CRUISE)
    OnSwitchClimbMode(false, Rate);

  // Calculate circling time percentage and call thermal band calculation
  PercentCircling(fabs(Rate));
}

void
GlideComputerAirData::ThermalSources()
{
  const DerivedInfo &calculated = Calculated();
  ThermalLocatorInfo &thermal_locator = SetCalculated().thermal_locator;

  if (!thermal_locator.estimate_valid ||
      !calculated.last_thermal.IsDefined())
    return;

  if (calculated.wind_available &&
      calculated.wind.norm / calculated.last_thermal.lift_rate > fixed(10.0)) {
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
    ThermalSource &source =
      thermal_locator.AllocateSource(Basic().time);

    source.lift_rate = calculated.last_thermal.lift_rate;
    source.location = ground_location;
    source.ground_height = ground_altitude;
    source.time = Basic().time;
  }
}

void
GlideComputerAirData::LastThermalStats()
{
  DerivedInfo &calculated = SetCalculated();

  if (calculated.circling != false ||
      LastCalculated().circling != true ||
      !positive(calculated.climb_start_time))
    return;

  fixed ThermalTime = calculated.cruise_start_time - calculated.climb_start_time;
  if (ThermalTime < THERMAL_TIME_MIN)
    return;

  fixed ThermalGain = calculated.cruise_start_altitude
    + Basic().EnergyHeight - calculated.climb_start_altitude;
  if (!positive(ThermalGain))
    return;

  bool was_defined = calculated.last_thermal.IsDefined();

  calculated.last_thermal.start_time = calculated.climb_start_time;
  calculated.last_thermal.end_time = calculated.cruise_start_time;
  calculated.last_thermal.gain = ThermalGain;
  calculated.last_thermal.duration = ThermalTime;
  calculated.last_thermal.CalculateLiftRate();

  if (!was_defined)
    calculated.last_thermal_average_smooth =
        calculated.last_thermal.lift_rate;
  else
    calculated.last_thermal_average_smooth =
        LowPassFilter(calculated.last_thermal_average_smooth,
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
  const DerivedInfo &calculated = Calculated();
  ThermalBandInfo &tbi = SetCalculated().thermal_band;

  const fixed h_safety = SettingsComputer().route_planner.safety_height_terrain +
    Calculated().GetTerrainBaseFallback();

  tbi.working_band_height = Basic().TEAltitude - h_safety;
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
                                      Basic().TEAltitude);
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
  if ((!Calculated().circling) || negative(Calculated().average))
    return;

  tbi.Add(dheight, Basic().BruttoVario);
}

void
GlideComputerAirData::ProcessSun()
{
  if (!Basic().location_available)
    return;

  DerivedInfo &calculated = SetCalculated();

  SunEphemeris sun;
  sun.CalcSunTimes(Basic().location, Basic().date_time_utc,
                   fixed(GetUTCOffset()) / 3600);
  calculated.sunset_time = fixed(sun.TimeOfSunSet);
  calculated.sun_azimuth = sun.Azimuth;
}
