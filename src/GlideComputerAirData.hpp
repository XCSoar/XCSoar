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
#include "ThermalLocator.hpp"
#include "Wind/WindAnalyser.hpp"
#include "GPSClock.hpp"
#include "Math/SunEphemeris.hpp"
#include "Util/WindowFilter.hpp"

class GlidePolar;
class ProtectedAirspaceWarningManager;
class ProtectedTaskManager;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class TaskClientCalc;

class GlideComputerAirData: virtual public GlideComputerBlackboard {
public:
  GlideComputerAirData(ProtectedAirspaceWarningManager &_awm,
                       ProtectedTaskManager &_task);

  GlideRatioCalculator rotaryLD;
  virtual void ProcessIdle();

  void SetWindEstimate(fixed wind_speed, Angle wind_bearing,
		       const int quality=3); // JMW check
  WindAnalyser   windanalyser; // JMW TODO, private and lock-protected

private:
  ThermalLocator thermallocator;
protected:
  ProtectedAirspaceWarningManager &m_airspace;

  void ResetFlight(const bool full=true);
  void ProcessBasic();
  void ProcessVertical();

  virtual void OnTakeoff();
  virtual void OnLanding();
  virtual void OnDepartedThermal();
  virtual void OnSwitchClimbMode(bool isclimb, bool left);

  bool FlightTimes();
private:
  void Heading();
  void Airspeed();
  void EnergyHeight();
  void BruttoVario();
  void NettoVario();
  void AverageClimbRate();
  void Average30s();
  void AverageThermal();
  void ResetLiftDatabase();
  void UpdateLiftDatabase();
  void MaxHeightGain();
  void ThermalGain();
  void LD();
  void CruiseLD();
  void Wind();

  /**
   * Choose a wind from: user input; external device; calculated.
   */
  void SelectWind();

  void TerrainHeight();
  void FlightState(const GlidePolar& glide_polar);
  void TakeoffLanding();
  void AirspaceWarning();
  void BallastDump();
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
  void PercentCircling(const fixed Rate);
  void TurnRate();
  void Turning();
  void ProcessSun();
  GPSClock airspace_clock;
  GPSClock ballast_clock;

  WindowFilter<30> vario_30s_filter;
  WindowFilter<30> netto_30s_filter;
};

#endif

