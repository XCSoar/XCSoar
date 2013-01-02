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

#if !defined(XCSOAR_GLIDECOMPUTER_AIRDATA_HPP)
#define XCSOAR_GLIDECOMPUTER_AIRDATA_HPP

#include "AutoQNH.hpp"
#include "GlideRatioCalculator.hpp"
#include "FlyingComputer.hpp"
#include "CirclingComputer.hpp"
#include "ThermalBandComputer.hpp"
#include "WindComputer.hpp"
#include "ThermalLocator.hpp"
#include "Math/WindowFilter.hpp"

struct VarioInfo;
struct OneClimbInfo;
struct TerrainInfo;
struct ThermalLocatorInfo;
struct ThermalBandInfo;
class Waypoints;
class Airspaces;
class RasterTerrain;
class GlidePolar;
class ProtectedAirspaceWarningManager;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputerAirData {
  const Waypoints &waypoints;
  const RasterTerrain *terrain;

  AutoQNH auto_qnh;

  GlideRatioCalculator gr_calculator;

  FlyingComputer flying_computer;
  CirclingComputer circling_computer;
  ThermalBandComputer thermal_band_computer;
  WindComputer wind_computer;

  ThermalLocator thermallocator;

  WindowFilter<30> vario_30s_filter;
  WindowFilter<30> netto_30s_filter;

public:
  GlideComputerAirData(const Waypoints &way_points);

  void SetTerrain(const RasterTerrain* _terrain) {
    terrain = _terrain;
  }

  const WindStore &GetWindStore() const {
    return wind_computer.GetWindStore();
  }

  void ResetFlight(DerivedInfo &calculated, const ComputerSettings &settings,
                   const bool full=true);

  /**
   * Calculates some basic values
   */
  void ProcessBasic(const MoreData &basic, DerivedInfo &calculated,
                    const ComputerSettings &settings);

  /**
   * Calculates some other values
   */
  void ProcessVertical(const MoreData &basic, const MoreData &last_basic,
                       DerivedInfo &calculated,
                       const DerivedInfo &last_calculated,
                       const ComputerSettings &settings);

protected:
  void OnSwitchClimbMode(const ComputerSettings &settings);

public:
  /**
   * 1. Detects time retreat and calls ResetFlight if GPS lost
   * 2. Detects change in replay status and calls ResetFlight if so
   * 3. Calls DetectStartTime and saves the time of flight
   * @return true as default, false if something is wrong in time
   */
  bool FlightTimes(const NMEAInfo &basic, const NMEAInfo &last_basic,
                   DerivedInfo &calculated,
                   const ComputerSettings &settings);

private:
  /**
   * Calculates the heading
   */
  void Heading(const NMEAInfo &basic, DerivedInfo &calculated);
  void EnergyHeight();
  void NettoVario(const NMEAInfo &basic, const FlyingState &flight,
                  VarioInfo &vario, const ComputerSettings &settings_computer);
  void AverageClimbRate(const NMEAInfo &basic, DerivedInfo &calculated);
  void Average30s(const MoreData &basic, const NMEAInfo &last_basic,
                  DerivedInfo &calculated, const DerivedInfo &last_calculated);
  void CurrentThermal(const MoreData &basic, const CirclingInfo &circling,
                      OneClimbInfo &current_thermal);
  void ResetLiftDatabase(DerivedInfo &calculated);
  void UpdateLiftDatabase(const MoreData &basic, DerivedInfo &calculated,
                          const DerivedInfo &last_calculated);
  void GR(const MoreData &basic, const MoreData &last_basic,
          const DerivedInfo &calculated, VarioInfo &vario_info);
  void CruiseGR(const MoreData &basic, DerivedInfo &calculated);

  void TerrainHeight(const MoreData &basic, TerrainInfo &calculated);
  void FlightState(const NMEAInfo &basic, const NMEAInfo &last_basic,
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
                        const DerivedInfo &last_calculated);

  /**
   * Calculates the turn rate and the derived features.
   * Determines the current flight mode (cruise/circling).
   */
  void Turning(const MoreData &basic, const MoreData &last_basic,
               DerivedInfo &calculated, const DerivedInfo &last_calculated,
               const ComputerSettings &settings);
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
