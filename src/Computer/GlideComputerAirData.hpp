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

#if !defined(XCSOAR_GLIDECOMPUTER_AIRDATA_HPP)
#define XCSOAR_GLIDECOMPUTER_AIRDATA_HPP

#include "GlideComputerBlackboard.hpp"
#include "GlideRatio.hpp"
#include "FlyingComputer.hpp"
#include "CirclingComputer.hpp"
#include "WindComputer.hpp"
#include "ThermalLocator.hpp"
#include "Util/WindowFilter.hpp"

class Waypoints;
class Airspaces;
class RasterTerrain;
class GlidePolar;
class ProtectedAirspaceWarningManager;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputerAirData: virtual public GlideComputerBlackboard {
  const Waypoints &way_points;
  const RasterTerrain *terrain;

  GlideRatioCalculator rotaryLD;

  FlyingComputer flying_computer;
  CirclingComputer circling_computer;
  WindComputer wind_computer;

  ThermalLocator thermallocator;

  WindowFilter<30> vario_30s_filter;
  WindowFilter<30> netto_30s_filter;

public:
  GlideComputerAirData(const Waypoints &way_points);

  void set_terrain(const RasterTerrain* _terrain) {
    terrain = _terrain;
  }

  const WindStore &GetWindStore() const {
    return wind_computer.GetWindStore();
  }

protected:
  void ResetFlight(const bool full=true);

  /**
   * Calculates some basic values
   */
  void ProcessBasic();

  /**
   * Calculates some other values
   */
  void ProcessVertical();

  virtual void OnTakeoff();
  virtual void OnLanding();

private:
  void OnDepartedThermal();

protected:
  virtual void OnSwitchClimbMode(bool isclimb, bool left);

  /**
   * 1. Detects time retreat and calls ResetFlight if GPS lost
   * 2. Detects change in replay status and calls ResetFlight if so
   * 3. Calls DetectStartTime and saves the time of flight
   * @return true as default, false if something is wrong in time
   */
  bool FlightTimes();

private:
  /**
   * Calculates the heading
   */
  void Heading();
  void EnergyHeight();
  void NettoVario();
  void AverageClimbRate();
  void Average30s();
  void CurrentThermal();
  void ResetLiftDatabase();
  void UpdateLiftDatabase();
  void MaxHeightGain();
  void LD();
  void CruiseLD();

  /**
   * Calculates the wind
   */
  void Wind();

  /**
   * Choose a wind from: user input; external device; calculated.
   */
  void SelectWind();

  void TerrainHeight();
  void FlightState(const GlidePolar& glide_polar);

  /**
   * Detects takeoff and landing events
   */
  void TakeoffLanding();
  void ThermalSources();

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
  void LastThermalStats();
  void WorkingBand();
  void ThermalBand();

  /**
   * Calculate the circling time percentage and call the thermal band calculation
   * @param Rate Current turn rate
   */
  void PercentCircling();

  /**
   * Calculates the turn rate
   */
  void TurnRate();

  /**
   * Calculates the turn rate and the derived features.
   * Determines the current flight mode (cruise/circling).
   */
  void Turning();
  void ProcessSun();
};

#endif
