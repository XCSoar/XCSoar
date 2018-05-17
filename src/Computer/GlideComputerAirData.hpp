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

#if !defined(XCSOAR_GLIDECOMPUTER_AIRDATA_HPP)
#define XCSOAR_GLIDECOMPUTER_AIRDATA_HPP

#include "AutoQNH.hpp"
#include "GlideRatioComputer.hpp"
#include "FlyingComputer.hpp"
#include "CirclingComputer.hpp"
#include "WaveComputer.hpp"
#include "ThermalBandComputer.hpp"
#include "Wind/Computer.hpp"
#include "LiftDatabaseComputer.hpp"
#include "AverageVarioComputer.hpp"
#include "ThermalLocator.hpp"

struct VarioInfo;
struct OneClimbInfo;
struct TerrainInfo;
struct ThermalLocatorInfo;
class Waypoints;
class RasterTerrain;
class GlidePolar;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputerAirData {
  const Waypoints &waypoints;
  const RasterTerrain *terrain;

  AutoQNH auto_qnh;

  GlideRatioComputer gr_computer;

  FlyingComputer flying_computer;
  CirclingComputer circling_computer;
  WaveComputer wave_computer;
  ThermalBandComputer thermal_band_computer;
  WindComputer wind_computer;
  LiftDatabaseComputer lift_database_computer;

  ThermalLocator thermallocator;

  AverageVarioComputer average_vario;

  /**
   * Used by FlightTimes() to detect time warps.
   */
  DeltaTime delta_time;

public:
  GlideComputerAirData(const Waypoints &way_points);

  void SetTerrain(const RasterTerrain* _terrain) {
    terrain = _terrain;
  }

  const WindStore &GetWindStore() const {
    return wind_computer.GetWindStore();
  }

  void ResetFlight(DerivedInfo &calculated, const bool full=true);

  void ResetStats() {
    circling_computer.ResetStats();
  }

  /**
   * Calculates some basic values
   */
  void ProcessBasic(const MoreData &basic, DerivedInfo &calculated,
                    const ComputerSettings &settings);

  /**
   * Calculates some other values
   */
  void ProcessVertical(const MoreData &basic,
                       DerivedInfo &calculated,
                       const ComputerSettings &settings);

  /**
   * 1. Detects time retreat and calls ResetFlight if GPS lost
   * 2. Detects change in replay status and calls ResetFlight if so
   * 3. Calls DetectStartTime and saves the time of flight
   */
  void FlightTimes(const NMEAInfo &basic,
                   DerivedInfo &calculated,
                   const ComputerSettings &settings);

private:
  void NettoVario(const NMEAInfo &basic, const FlyingState &flight,
                  VarioInfo &vario, const ComputerSettings &settings_computer);
  void AverageClimbRate(const NMEAInfo &basic, DerivedInfo &calculated);
  void CurrentThermal(const MoreData &basic, const CirclingInfo &circling,
                      OneClimbInfo &current_thermal);
  void GR(const MoreData &basic, const FlyingState &flying,
          VarioInfo &vario_info);
  void CruiseGR(const MoreData &basic, DerivedInfo &calculated);

  void TerrainHeight(const MoreData &basic, TerrainInfo &calculated);
  void FlightState(const NMEAInfo &basic,
                   const DerivedInfo &calculated, FlyingState &flying,
                   const GlidePolar &glide_polar);

   void ThermalSources(const MoreData &basic, const DerivedInfo &calculated,
                       ThermalLocatorInfo &thermal_locator);

  /**
   * Updates stats during transition from climb mode to cruise mode
   * Sets last thermal stats if Circling state has changed
   * Inputs:
   *  CruiseStartTime
   *  ClimbStartTime
   *  CruiseStartAlt
   *  Basic().EnergyHeight
   * Updates:
   *  LastThermalAverage
   *  LastThermalGain
   *  LastThermalTime
   *  LastThermalAverageSmooth
   */
  void LastThermalStats(const MoreData &basic, DerivedInfo &calculated,
                        bool last_circling);

  /**
   * Calculates the turn rate and the derived features.
   * Determines the current flight mode (cruise/circling).
   */
  void Turning(const MoreData &basic,
               DerivedInfo &calculated, const ComputerSettings &settings);
  void ProcessSun(const NMEAInfo &basic, DerivedInfo &calculated,
                  const ComputerSettings &settings);

  /**
   * Calculates the thermal value of next leg that is equivalent (gives the
   * same average speed) to the current MacCready setting.
   */
  void NextLegEqThermal(const NMEAInfo &basic, DerivedInfo &calculated,
                        const ComputerSettings &settings);
};

#endif
