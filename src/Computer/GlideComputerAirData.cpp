/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Settings.hpp"
#include "Math/LowPassFilter.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "ThermalBase.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Math/SunEphemeris.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"

static constexpr double THERMAL_TIME_MIN = 45;
static constexpr double THERMAL_SHEAR_RATIO_MAX = 10;
static constexpr double DEFAULT_TAKEOFF_SPEED = 10;
static constexpr double CLIMB_RATE_G_MIN = 0.25;
static constexpr double LOW_PASS_FILTER_VARIO_LD_ALPHA = 0.3;
static constexpr double LOW_PASS_FILTER_THERMAL_AVERAGE_ALPHA = 0.3;

GlideComputerAirData::GlideComputerAirData(const Waypoints &_way_points)
  :waypoints(_way_points),
   terrain(NULL)
{
  // JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing, 1);
}

void
GlideComputerAirData::ResetFlight(DerivedInfo &calculated,
                                  const bool full)
{
  auto_qnh.Reset();

  average_vario.Reset();

  lift_database_computer.Reset(calculated.lift_database,
                               calculated.trace_history.CirclingAverage);

  thermallocator.Reset();

  gr_computer.Reset();

  if (full)
    flying_computer.Reset();

  circling_computer.Reset();
  wave_computer.Reset();

  thermal_band_computer.Reset();
  wind_computer.Reset();

  delta_time.Reset();
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
                                      DerivedInfo &calculated,
                                      const ComputerSettings &settings)
{
  /* the "circling" flag may be modified by
     CirclingComputer::Turning(); remember the old state so this
     method can check for modifications */
  const bool last_circling = calculated.circling;

  auto_qnh.Process(basic, calculated, settings, waypoints);

  circling_computer.TurnRate(calculated, basic,
                             calculated.flight);
  Turning(basic, calculated, settings);

  wave_computer.Compute(basic, calculated.flight,
                        calculated.wave, settings.wave);

  wind_computer.Compute(settings.wind, settings.polar.glide_polar_task,
                        basic, calculated);
  wind_computer.Select(settings.wind, basic, calculated);
  wind_computer.ComputeHeadWind(basic, calculated);

  thermallocator.Process(calculated.circling && calculated.turning,
                         basic.time, basic.location,
                         basic.netto_vario,
                         calculated.GetWindOrZero(),
                         calculated.thermal_locator);

  LastThermalStats(basic, calculated, last_circling);

  gr_computer.Compute(basic, calculated,
                      calculated,
                      settings);

  GR(basic, calculated.flight, calculated);
  CruiseGR(basic, calculated);

  average_vario.Compute(basic, calculated.circling, last_circling,
                        calculated);
  AverageClimbRate(basic, calculated);

  if (calculated.circling)
    CurrentThermal(basic, calculated, calculated.current_thermal);
  else
    calculated.current_thermal = calculated.last_thermal;

  lift_database_computer.Compute(calculated.lift_database,
                                 calculated.trace_history.CirclingAverage,
                                 basic, calculated);
  circling_computer.MaxHeightGain(basic, calculated.flight, calculated);
  NextLegEqThermal(basic, calculated, settings);
}

inline void
GlideComputerAirData::NettoVario(const NMEAInfo &basic,
                                 const FlyingState &flight,
                                 VarioInfo &vario,
                                 const ComputerSettings &settings_computer)
{
  auto g_load = basic.acceleration.available
    ? basic.acceleration.g_load
    : 1;

  vario.sink_rate =
    flight.flying && basic.airspeed_available &&
    settings_computer.polar.glide_polar_task.IsValid()
    ? - settings_computer.polar.glide_polar_task.SinkRate(basic.indicated_airspeed,
                                                          g_load)
    /* the glider sink rate is useless when not flying */
    : 0;
}

inline void
GlideComputerAirData::AverageClimbRate(const NMEAInfo &basic,
                                       DerivedInfo &calculated)
{
  if (basic.airspeed_available && basic.indicated_airspeed > 0 &&
      basic.true_airspeed > 0 &&
      basic.total_energy_vario_available &&
      !calculated.circling &&
      (!basic.acceleration.available ||
       !basic.acceleration.real ||
       fabs(basic.acceleration.g_load - 1) <= CLIMB_RATE_G_MIN)) {
    // TODO: Check this is correct for TAS/IAS
    auto ias_to_tas = basic.indicated_airspeed / basic.true_airspeed;
    auto w_tas = basic.total_energy_vario * ias_to_tas;

    calculated.climb_history.Add(uround(basic.indicated_airspeed), w_tas);
  }
}

inline void
GlideComputerAirData::CurrentThermal(const MoreData &basic,
                                     const CirclingInfo &circling,
                                     OneClimbInfo &current_thermal)
{
  if (circling.climb_start_time > 0) {
    current_thermal.start_time = circling.climb_start_time;
    current_thermal.end_time = basic.time;
    current_thermal.gain =
      basic.TE_altitude - circling.climb_start_altitude_te;
    current_thermal.CalculateAll();
  } else
    current_thermal.Clear();
}


inline void
GlideComputerAirData::GR(const MoreData &basic, const FlyingState &flying,
                         VarioInfo &vario_info)
{
  // Lift / drag instantaneous from vario, updated every reading..
  if (basic.total_energy_vario_available && basic.airspeed_available &&
      flying.flying) {
    vario_info.ld_vario =
      UpdateGR(vario_info.ld_vario, basic.indicated_airspeed,
               -basic.total_energy_vario, LOW_PASS_FILTER_VARIO_LD_ALPHA);
  } else {
    vario_info.ld_vario = INVALID_GR;
  }
}

inline void
GlideComputerAirData::CruiseGR(const MoreData &basic, DerivedInfo &calculated)
{
  if (!calculated.circling && basic.NavAltitudeAvailable()) {
    if (calculated.cruise_start_time < 0) {
      calculated.cruise_start_location = basic.location;
      calculated.cruise_start_altitude = basic.nav_altitude;
      calculated.cruise_start_time = basic.time;
    } else {
      auto DistanceFlown =
        basic.location.DistanceS(calculated.cruise_start_location);

      calculated.cruise_gr =
          UpdateGR(calculated.cruise_gr, DistanceFlown,
                   calculated.cruise_start_altitude - basic.nav_altitude,
                   0.5);
    }
  }
}

/**
 * Reads the current terrain height
 */
inline void
GlideComputerAirData::TerrainHeight(const MoreData &basic,
                                    TerrainInfo &calculated)
{
  if (!basic.location_available || terrain == NULL) {
    calculated.terrain_valid = false;
    calculated.terrain_altitude = 0;
    calculated.altitude_agl_valid = false;
    calculated.altitude_agl = 0;
    return;
  }

  const auto h = terrain->GetTerrainHeight(basic.location);
  if (h.IsInvalid()) {
    calculated.terrain_valid = false;
    calculated.terrain_altitude = 0;
    calculated.altitude_agl_valid = false;
    calculated.altitude_agl = 0;
    return;
  }

  calculated.terrain_valid = true;
  calculated.terrain_altitude = h.GetValueOr0();

  if (basic.NavAltitudeAvailable()) {
    calculated.altitude_agl = basic.nav_altitude - calculated.terrain_altitude;
    calculated.altitude_agl_valid = true;
  } else
    calculated.altitude_agl_valid = false;
}

void
GlideComputerAirData::FlightTimes(const NMEAInfo &basic,
                                  DerivedInfo &calculated,
                                  const ComputerSettings &settings)
{
  if (basic.time_available &&
      delta_time.Update(basic.time, 0, 180) < 0)
    /* time warp: reset the computer */
    ResetFlight(calculated, true);

  FlightState(basic, calculated, calculated.flight,
              settings.polar.glide_polar_task);
}

inline void
GlideComputerAirData::FlightState(const NMEAInfo &basic,
                                  const DerivedInfo &calculated,
                                  FlyingState &flying,
                                  const GlidePolar &glide_polar)
{
  auto v_takeoff = glide_polar.IsValid()
    ? glide_polar.GetVTakeoff()
    /* if there's no valid polar, assume 10 m/s (36 km/h); that's an
       arbitrary value, but better than nothing */
    : DEFAULT_TAKEOFF_SPEED;

  flying_computer.Compute(v_takeoff, basic,
                          calculated, flying);
}

inline void
GlideComputerAirData::Turning(const MoreData &basic,
                              DerivedInfo &calculated,
                              const ComputerSettings &settings)
{
  circling_computer.Turning(calculated,
                            basic,
                            calculated.flight,
                            settings.circling);

  // Calculate circling time percentage and call thermal band calculation
  circling_computer.PercentCircling(basic, calculated.flight, calculated);

  thermal_band_computer.Compute(basic, calculated,
                                calculated.thermal_encounter_band,
                                calculated.thermal_encounter_collection,
                                settings);
}

inline void
GlideComputerAirData::ThermalSources(const MoreData &basic,
                                     const DerivedInfo &calculated,
                                     ThermalLocatorInfo &thermal_locator)
{
  if (!thermal_locator.estimate_valid ||
      !basic.NavAltitudeAvailable() ||
      !calculated.last_thermal.IsDefined())
    return;

  if (calculated.wind_available &&
      calculated.wind.norm / calculated.last_thermal.lift_rate > THERMAL_SHEAR_RATIO_MAX) {
    // thermal strength is so weak compared to wind that source estimate
    // is unlikely to be reliable, so don't calculate or remember it
    return;
  }

  GeoPoint ground_location;
  double ground_altitude = -1;
  EstimateThermalBase(terrain, thermal_locator.estimate_location,
                      basic.nav_altitude,
                      calculated.last_thermal.lift_rate,
                      calculated.GetWindOrZero(),
                      ground_location,
                      ground_altitude);

  if (ground_altitude > 0) {
    ThermalSource &source = thermal_locator.AllocateSource();

    source.lift_rate = calculated.last_thermal.lift_rate;
    source.location = ground_location;
    source.ground_height = ground_altitude;
    source.time = basic.time;
  }
}

inline void
GlideComputerAirData::LastThermalStats(const MoreData &basic,
                                       DerivedInfo &calculated,
                                       bool last_circling)
{
  if (calculated.circling || !last_circling ||
      calculated.climb_start_time <= 0)
    return;

  auto duration = calculated.cruise_start_time - calculated.climb_start_time;
  if (duration < THERMAL_TIME_MIN)
    return;

  auto gain = calculated.cruise_start_altitude_te
    - calculated.climb_start_altitude_te;

  if (gain <= 0)
    return;

  bool was_defined = calculated.last_thermal.IsDefined();

  calculated.last_thermal.start_time = calculated.climb_start_time;
  calculated.last_thermal.end_time = calculated.cruise_start_time;
  calculated.last_thermal.gain = gain;
  calculated.last_thermal.duration = duration;
  calculated.last_thermal.start_altitude = calculated.climb_start_altitude_te + (basic.nav_altitude-basic.TE_altitude);
  calculated.last_thermal.CalculateLiftRate();
  assert(calculated.last_thermal.lift_rate > 0);

  if (!was_defined)
    calculated.last_thermal_average_smooth =
        calculated.last_thermal.lift_rate;
  else
    calculated.last_thermal_average_smooth =
        LowPassFilter(calculated.last_thermal_average_smooth,
                      calculated.last_thermal.lift_rate, LOW_PASS_FILTER_THERMAL_AVERAGE_ALPHA);

  ThermalSources(basic, calculated, calculated.thermal_locator);
}

inline void
GlideComputerAirData::ProcessSun(const NMEAInfo &basic,
                                 DerivedInfo &calculated,
                                 const ComputerSettings &settings)
{
  if (!basic.location_available || !basic.date_time_utc.IsDatePlausible())
    return;

  // Only calculate new azimuth if data is older than 15 minutes
  if (!calculated.sun_data_available.IsOlderThan(basic.clock, 15 * 60))
    return;

  // Calculate new azimuth
  calculated.sun_azimuth =
    SunEphemeris::CalcAzimuth(basic.location, basic.date_time_utc,
                              settings.utc_offset);
  calculated.sun_data_available.Update(basic.clock);
}

inline void
GlideComputerAirData::NextLegEqThermal(const NMEAInfo &basic,
                                       DerivedInfo &calculated,
                                       const ComputerSettings &settings)
{
  const GeoVector vector_remaining =
      calculated.task_stats.current_leg.vector_remaining;
  const GeoVector next_leg_vector =
      calculated.task_stats.current_leg.next_leg_vector;

  if (!settings.polar.glide_polar_task.IsValid() ||
      !next_leg_vector.IsValid() ||
      !vector_remaining.IsValid() ||
      !calculated.wind_available) {
    // Assign a negative value to invalidate the result
    calculated.next_leg_eq_thermal = -1;
    return;
  }

  // Calculate wind component on current and next legs
  const auto wind_comp = calculated.wind.norm *
      (calculated.wind.bearing - vector_remaining.bearing).fastcosine();
  const auto next_comp = calculated.wind.norm *
      (calculated.wind.bearing - next_leg_vector.bearing).fastcosine();

  calculated.next_leg_eq_thermal =
      settings.polar.glide_polar_task.GetNextLegEqThermal(wind_comp, next_comp);
}
