/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "ComputerSettings.hpp"
#include "Math/LowPassFilter.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "ThermalBase.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "NMEA/Aircraft.hpp"
#include "Math/SunEphemeris.hpp"

#include <algorithm>

using std::min;
using std::max;

static const fixed THERMAL_TIME_MIN(45);

GlideComputerAirData::GlideComputerAirData(const Waypoints &_way_points)
  :waypoints(_way_points),
   terrain(NULL)
{
  // JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing, 1);
}

void
GlideComputerAirData::ResetFlight(DerivedInfo &calculated,
                                  const ComputerSettings &settings,
                                  const bool full)
{
  auto_qnh.Reset();

  vario_30s_filter.Reset();
  netto_30s_filter.Reset();

  ResetLiftDatabase(calculated);

  thermallocator.Reset();

  gr_calculator.Initialize(settings);

  if (full)
    flying_computer.Reset();

  thermal_band_computer.Reset();
  wind_computer.Reset();
}

void
GlideComputerAirData::ProcessBasic(const MoreData &basic,
                                   DerivedInfo &calculated,
                                   const ComputerSettings &settings)
{
  TerrainHeight(basic, calculated);
  ProcessSun(basic, calculated, settings);

  NettoVario(basic, calculated.flight, calculated, settings);
}

void
GlideComputerAirData::ProcessVertical(const MoreData &basic,
                                      const MoreData &last_basic,
                                      DerivedInfo &calculated,
                                      const DerivedInfo &last_calculated,
                                      const ComputerSettings &settings)
{
  auto_qnh.Process(basic, calculated, settings, waypoints);

  Heading(basic, calculated);
  circling_computer.TurnRate(calculated, basic, last_basic,
                             calculated, last_calculated);
  Turning(basic, last_basic, calculated, last_calculated, settings);

  wind_computer.Compute(settings.wind, settings.polar.glide_polar_task,
                        basic, calculated);
  wind_computer.Select(settings.wind, basic, calculated);
  wind_computer.ComputeHeadWind(basic, calculated);

  thermallocator.Process(calculated.circling,
                         basic.time, basic.location,
                         basic.netto_vario,
                         calculated.GetWindOrZero(),
                         calculated.thermal_locator);

  LastThermalStats(basic, calculated, last_calculated);
  GR(basic, last_basic, calculated, calculated);
  CruiseGR(basic, calculated);

  if (calculated.flight.flying && !calculated.circling)
    calculated.average_gr = gr_calculator.Calculate();

  Average30s(basic, last_basic, calculated, last_calculated);
  AverageClimbRate(basic, calculated);
  CurrentThermal(basic, calculated, calculated.current_thermal);
  UpdateLiftDatabase(basic, calculated, last_calculated);
  circling_computer.MaxHeightGain(basic, calculated.flight, calculated);
  NextLegEqThermal(basic, calculated, settings);
}

void
GlideComputerAirData::Heading(const NMEAInfo &basic, DerivedInfo &calculated)
{
  const SpeedVector wind = calculated.wind;

  if (basic.attitude.heading_available) {
    calculated.heading = basic.attitude.heading;
  } else if (calculated.wind_available &&
             (positive(basic.ground_speed) || wind.IsNonZero()) &&
             calculated.flight.flying) {
    fixed x0 = basic.track.fastsine() * basic.ground_speed;
    fixed y0 = basic.track.fastcosine() * basic.ground_speed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    calculated.heading = Angle::Radians(atan2(x0, y0)).AsBearing();
  } else {
    calculated.heading = basic.track;
  }
}

void
GlideComputerAirData::NettoVario(const NMEAInfo &basic,
                                 const FlyingState &flight,
                                 VarioInfo &vario,
                                 const ComputerSettings &settings_computer)
{
  fixed g_load = basic.acceleration.available ?
                 basic.acceleration.g_load : fixed_one;

  vario.sink_rate =
    flight.flying && basic.airspeed_available &&
    settings_computer.polar.glide_polar_task.IsValid()
    ? - settings_computer.polar.glide_polar_task.SinkRate(basic.indicated_airspeed,
                                                          g_load)
    /* the glider sink rate is useless when not flying */
    : fixed_zero;
}

void
GlideComputerAirData::AverageClimbRate(const NMEAInfo &basic,
                                       DerivedInfo &calculated)
{
  if (basic.airspeed_available && positive(basic.indicated_airspeed) &&
      positive(basic.true_airspeed) &&
      basic.total_energy_vario_available &&
      !calculated.circling &&
      (!basic.acceleration.available ||
       !basic.acceleration.real ||
       fabs(basic.acceleration.g_load - fixed_one) <= fixed(0.25))) {
    // TODO: Check this is correct for TAS/IAS
    fixed ias_to_tas = basic.indicated_airspeed / basic.true_airspeed;
    fixed w_tas = basic.total_energy_vario * ias_to_tas;

    calculated.climb_history.Add(uround(basic.indicated_airspeed), w_tas);
  }
}

void
GlideComputerAirData::Average30s(const MoreData &basic,
                                 const NMEAInfo &last_basic,
                                 DerivedInfo &calculated,
                                 const DerivedInfo &last_calculated)
{
  const bool time_advanced = basic.HasTimeAdvancedSince(last_basic);
  if (!time_advanced || calculated.circling != last_calculated.circling) {
    vario_30s_filter.Reset();
    netto_30s_filter.Reset();
    calculated.average = basic.brutto_vario;
    calculated.netto_average = basic.netto_vario;
  }

  if (!time_advanced)
    return;

  const unsigned Elapsed(basic.time - last_basic.time);
  if (Elapsed == 0)
    return;

  for (unsigned i = 0; i < Elapsed; ++i) {
    vario_30s_filter.Update(basic.brutto_vario);
    netto_30s_filter.Update(basic.netto_vario);
  }
  calculated.average = vario_30s_filter.Average();
  calculated.netto_average = netto_30s_filter.Average();
}

void
GlideComputerAirData::CurrentThermal(const MoreData &basic,
                                     const CirclingInfo &circling,
                                     OneClimbInfo &current_thermal)
{
  if (positive(circling.climb_start_time)) {
    current_thermal.start_time = circling.climb_start_time;
    current_thermal.end_time = basic.time;
    current_thermal.gain = basic.TE_altitude - circling.climb_start_altitude;
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
  static const Angle afive = Angle::Degrees(fixed(5));

  unsigned index = (unsigned)
      floor((heading + afive).AsBearing().Degrees() / 10);

  return std::max(0u, std::min(35u, index));
}

void
GlideComputerAirData::UpdateLiftDatabase(const MoreData &basic,
                                         DerivedInfo &calculated,
                                         const DerivedInfo &last_calculated)
{
  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!calculated.circling && last_calculated.circling)
    ResetLiftDatabase(calculated);

  // Determine the direction in which we are circling
  bool left = calculated.TurningLeft();

  // Depending on the direction set the step size sign for the
  // following loop
  Angle heading_step = Angle::Degrees(fixed(left ? -10 : 10));

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
  for (Angle h = last_calculated.heading;
       left == negative((calculated.heading - h).AsDelta().Degrees());
       h += heading_step) {
    unsigned index = heading_to_index(h);
    calculated.lift_database[index] = basic.brutto_vario;
  }

  // detect zero crossing
  if (((calculated.heading.Degrees()< fixed_90) && 
       (last_calculated.heading.Degrees()> fixed_270)) ||
      ((last_calculated.heading.Degrees()< fixed_90) && 
       (calculated.heading.Degrees()> fixed_270))) {

    fixed h_av = fixed_zero;
    for (auto it = calculated.lift_database.begin(),
         it_end = calculated.lift_database.end(); it != it_end; ++it)
      h_av += *it;

    h_av /= calculated.lift_database.size();
    calculated.trace_history.CirclingAverage.push(h_av);
  }
}

void
GlideComputerAirData::ResetLiftDatabase(DerivedInfo &calculated)
{
  calculated.ClearLiftDatabase();

  calculated.trace_history.CirclingAverage.clear();
}

void
GlideComputerAirData::GR(const MoreData &basic, const MoreData &last_basic,
                         const DerivedInfo &calculated, VarioInfo &vario_info)
{
  if (!basic.NavAltitudeAvailable() || !last_basic.NavAltitudeAvailable()) {
    vario_info.ld_vario = INVALID_GR;
    vario_info.gr = INVALID_GR;
    return;
  }

  if (basic.HasTimeRetreatedSince(last_basic)) {
    vario_info.ld_vario = INVALID_GR;
    vario_info.gr = INVALID_GR;
  }

  const bool time_advanced = basic.HasTimeAdvancedSince(last_basic);
  if (time_advanced) {
    fixed DistanceFlown = basic.location.Distance(last_basic.location);

    // Glide ratio over ground
    vario_info.gr =
      UpdateGR(vario_info.gr, DistanceFlown,
               last_basic.nav_altitude - basic.nav_altitude, fixed(0.1));

    if (calculated.flight.flying && !calculated.circling)
      gr_calculator.Add((int)DistanceFlown, (int)basic.nav_altitude);
  }

  // Lift / drag instantaneous from vario, updated every reading..
  if (basic.total_energy_vario_available && basic.airspeed_available &&
      calculated.flight.flying) {
    vario_info.ld_vario =
      UpdateGR(vario_info.ld_vario, basic.indicated_airspeed,
               -basic.total_energy_vario, fixed(0.3));
  } else {
    vario_info.ld_vario = INVALID_GR;
  }
}

void
GlideComputerAirData::CruiseGR(const MoreData &basic, DerivedInfo &calculated)
{
  if (!calculated.circling && basic.NavAltitudeAvailable()) {
    if (negative(calculated.cruise_start_time)) {
      calculated.cruise_start_location = basic.location;
      calculated.cruise_start_altitude = basic.nav_altitude;
      calculated.cruise_start_time = basic.time;
    } else {
      fixed DistanceFlown =
        basic.location.Distance(calculated.cruise_start_location);

      calculated.cruise_gr =
          UpdateGR(calculated.cruise_gr, DistanceFlown,
                   calculated.cruise_start_altitude - basic.nav_altitude,
                   fixed_half);
    }
  }
}

/**
 * Reads the current terrain height
 */
void
GlideComputerAirData::TerrainHeight(const MoreData &basic,
                                    TerrainInfo &calculated)
{
  if (!basic.location_available || terrain == NULL) {
    calculated.terrain_valid = false;
    calculated.terrain_altitude = fixed_zero;
    calculated.altitude_agl_valid = false;
    calculated.altitude_agl = fixed_zero;
    return;
  }

  short Alt = terrain->GetTerrainHeight(basic.location);
  if (RasterBuffer::IsSpecial(Alt)) {
    if (RasterBuffer::IsWater(Alt))
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

  if (basic.NavAltitudeAvailable()) {
    calculated.altitude_agl = basic.nav_altitude - calculated.terrain_altitude;
    calculated.altitude_agl_valid = true;
  } else
    calculated.altitude_agl_valid = false;
}

bool
GlideComputerAirData::FlightTimes(const NMEAInfo &basic,
                                  const NMEAInfo &last_basic,
                                  DerivedInfo &calculated,
                                  const ComputerSettings &settings)
{
  if (basic.gps.replay != last_basic.gps.replay)
    // reset flight before/after replay logger
    ResetFlight(calculated, settings, basic.gps.replay);

  if (basic.time_available && basic.HasTimeRetreatedSince(last_basic)) {
    // 20060519:sgi added (basic.Time != 0) due to always return here
    // if no GPS time available
    if (basic.location_available)
      // Reset statistics.. (probably due to being in IGC replay mode)
      ResetFlight(calculated, settings, false);

    return false;
  }

  FlightState(basic, last_basic, calculated, calculated.flight,
              settings.polar.glide_polar_task);

  return true;
}

void
GlideComputerAirData::FlightState(const NMEAInfo &basic,
                                  const NMEAInfo &last_basic,
                                  const DerivedInfo &calculated,
                                  FlyingState &flying,
                                  const GlidePolar &glide_polar)
{
  fixed v_takeoff = glide_polar.IsValid()
    ? glide_polar.GetVTakeoff()
    /* if there's no valid polar, assume 10 m/s (36 km/h); that's an
       arbitrary value, but better than nothing */
    : fixed(10);

  flying_computer.Compute(v_takeoff, basic, last_basic,
                          calculated, flying);
}

void
GlideComputerAirData::OnSwitchClimbMode(const ComputerSettings &settings)
{
  gr_calculator.Initialize(settings);
}

void
GlideComputerAirData::Turning(const MoreData &basic,
                              const MoreData &last_basic,
                              DerivedInfo &calculated,
                              const DerivedInfo &last_calculated,
                              const ComputerSettings &settings)
{
  circling_computer.Turning(calculated,
                            basic, last_basic,
                            calculated, last_calculated,
                            settings);

  if (last_calculated.circling != calculated.circling)
    OnSwitchClimbMode(settings);

  // Calculate circling time percentage and call thermal band calculation
  circling_computer.PercentCircling(basic, last_basic, calculated);

  thermal_band_computer.Compute(basic, calculated,
                                calculated.thermal_band,
                                settings);
}

void
GlideComputerAirData::ThermalSources(const MoreData &basic,
                                     const DerivedInfo &calculated,
                                     ThermalLocatorInfo &thermal_locator)
{
  if (!thermal_locator.estimate_valid ||
      !basic.NavAltitudeAvailable() ||
      !calculated.last_thermal.IsDefined() ||
      negative(calculated.last_thermal.lift_rate))
    return;

  if (calculated.wind_available &&
      calculated.wind.norm / calculated.last_thermal.lift_rate > fixed(10.0)) {
    // thermal strength is so weak compared to wind that source estimate
    // is unlikely to be reliable, so don't calculate or remember it
    return;
  }

  GeoPoint ground_location;
  fixed ground_altitude = fixed_minus_one;
  EstimateThermalBase(terrain, thermal_locator.estimate_location,
                      basic.nav_altitude,
                      calculated.last_thermal.lift_rate,
                      calculated.GetWindOrZero(),
                      ground_location,
                      ground_altitude);

  if (positive(ground_altitude)) {
    ThermalSource &source = thermal_locator.AllocateSource();

    source.lift_rate = calculated.last_thermal.lift_rate;
    source.location = ground_location;
    source.ground_height = ground_altitude;
    source.time = basic.time;
  }
}

void
GlideComputerAirData::LastThermalStats(const MoreData &basic,
                                       DerivedInfo &calculated,
                                       const DerivedInfo &last_calculated)
{
  if (calculated.circling != false ||
      last_calculated.circling != true ||
      !positive(calculated.climb_start_time))
    return;

  fixed duration = calculated.cruise_start_time - calculated.climb_start_time;
  if (duration < THERMAL_TIME_MIN)
    return;

  fixed gain = calculated.cruise_start_altitude
    + basic.energy_height - calculated.climb_start_altitude;
  if (!positive(gain))
    return;

  bool was_defined = calculated.last_thermal.IsDefined();

  calculated.last_thermal.start_time = calculated.climb_start_time;
  calculated.last_thermal.end_time = calculated.cruise_start_time;
  calculated.last_thermal.gain = gain;
  calculated.last_thermal.duration = duration;
  calculated.last_thermal.CalculateLiftRate();

  if (!was_defined)
    calculated.last_thermal_average_smooth =
        calculated.last_thermal.lift_rate;
  else
    calculated.last_thermal_average_smooth =
        LowPassFilter(calculated.last_thermal_average_smooth,
                      calculated.last_thermal.lift_rate, fixed(0.3));

  ThermalSources(basic, calculated, calculated.thermal_locator);
}

void
GlideComputerAirData::ProcessSun(const NMEAInfo &basic,
                                 DerivedInfo &calculated,
                                 const ComputerSettings &settings)
{
  if (!basic.location_available || !basic.date_available)
    return;

  // Only calculate new azimuth if data is older than 15 minutes
  if (!calculated.sun_data_available.IsOlderThan(basic.clock, fixed(15 * 60)))
    return;

  // Calculate new azimuth
  calculated.sun_azimuth =
    SunEphemeris::CalcAzimuth(basic.location, basic.date_time_utc,
                              fixed(settings.utc_offset) / 3600);
  calculated.sun_data_available.Update(basic.clock);
}

void
GlideComputerAirData::NextLegEqThermal(const NMEAInfo &basic,
                                       DerivedInfo &calculated,
                                       const ComputerSettings &settings)
{
  const GeoVector vector_remaining =
      calculated.task_stats.current_leg.vector_remaining;
  const GeoVector next_leg_vector =
      calculated.task_stats.current_leg.next_leg_vector;

  if(!next_leg_vector.IsValid() ||
      !vector_remaining.IsValid() ||
      !calculated.wind_available) {
    // Assign a negative value to invalidate the result
    calculated.next_leg_eq_thermal = fixed_minus_one;
    return;
  }

  // Calculate wind component on current and next legs
  const fixed wind_comp = calculated.wind.norm *
      (calculated.wind.bearing - vector_remaining.bearing).fastcosine();
  const fixed next_comp = calculated.wind.norm *
      (calculated.wind.bearing - next_leg_vector.bearing).fastcosine();

  calculated.next_leg_eq_thermal =
      settings.polar.glide_polar_task.GetNextLegEqThermal(wind_comp, next_comp);
}
