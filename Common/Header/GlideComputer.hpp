/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#if !defined(XCSOAR_GLIDECOMPUTER_HPP)
#define XCSOAR_GLIDECOMPUTER_HPP

#include "Blackboard.hpp"
#include "FlightStatistics.hpp"
#include "AATDistance.h"
#include "OnLineContest.h"
#include "Audio/VegaVoice.h"
#include "GlideRatio.hpp"
#include "ThermalLocator.h"
#include "windanalyser.h"
#include "SnailTrail.hpp"

class MapWindowProjection;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?


class GlideComputerAirData: virtual public GlideComputerBlackboard {
public:
  GlideComputerAirData();
  ldrotary_s           rotaryLD;
  ThermalLocator thermallocator;
  WindAnalyser   windanalyser;
  virtual void ProcessIdle(const MapWindowProjection &map);

  void SetWindEstimate(const double wind_speed,
		       const double wind_bearing,
		       const int quality=3); // JMW check

protected:

  void ResetFlight(const bool full=true);
  void Initialise();
  void ProcessBasic();
  void ProcessVertical();

  virtual bool ProcessVario();

  void DoWindCirclingMode(const bool left);
  void DoWindCirclingSample();
  void DoWindCirclingAltitude();
  bool FlightTimes();
private:
  void AverageClimbRate();
  void Average30s();
  void AverageThermal();
  void MaxHeightGain();
  void ThermalGain();
  void LD();
  void CruiseLD();
  void Heading();
  void DoWindZigZag();
  void TerrainHeight();
  void EnergyHeightNavAltitude();
  void Vario();
  void SpeedToFly(const double mc_setting, 
		  const double cruise_efficiency);
  void NettoVario();
  void TakeoffLanding();
  void OnTakeoff();
  void OnLanding();
  void PredictNextPosition();
  void AirspaceWarning(const MapWindowProjection &map_projection);
  void TerrainFootprint(const double max_dist);
  void BallastDump();
  void ThermalSources();
  void LastThermalStats();
  void ThermalBand();
  void PercentCircling(const double Rate);
  void SwitchZoomClimb(bool isclimb, bool left);
  void Turning();
};

class GlideComputerStats: virtual public GlideComputerBlackboard {
public:
  FlightStatistics     flightstats;
  SnailTrail           snail_trail;
protected:
  void ResetFlight(const bool full=true);
  void StartTask();
  void Initialise();
  bool DoLogging();
  void SetFastLogging();
  virtual double GetAverageThermal() const;
protected:
  virtual void SaveTaskSpeed(double val);
  virtual void SetLegStart();
  virtual void OnClimbBase(double StartAlt);
  virtual void OnClimbCeiling();
  virtual void OnDepartedThermal();
};

class GlideComputerTask: virtual public GlideComputerBlackboard {
public:
  AATDistance          aatdistance;
  OLCOptimizer         olc;
  void DoAutoMacCready(double mc_setting);
  virtual bool InsideStartHeight(const DWORD Margin=0) const;
  virtual bool ValidStartSpeed(const DWORD Margin=0) const;
protected:
  void Initialise() {}
  void ProcessBasicTask(const double mc_setting, 
			const double cruise_efficiency);
  void ResetFlight(const bool full=true);
  virtual void StartTask(const bool do_advance,
			 const bool do_announce);
  virtual void AnnounceWayPointSwitch(bool do_advance)= 0;
  bool DoLogging();
  void InSector();

  // abort stuff
  void SortLandableWaypoints();
private:
  void DistanceToHome();
  void DistanceToNext();
  void AltitudeRequired(const double mc_setting,
			const double cruise_efficiency);
  double AATCloseBearing() const;
  bool InTurnSector(const int the_turnpoint) const;
  bool InFinishSector(const int i);
  bool ValidFinish() const;
  bool InStartSector_Internal(int Index,
			      double OutBound,
			      bool &LastInSector);
  bool InStartSector(int &index,
		     BOOL *CrossedStart);
  bool ReadyToStart();
  bool ReadyToAdvance(bool reset=true, bool restart=false);
  void CheckStart(int *LastStartSector);
  BOOL CheckRestart(int *LastStartSector);
  void CheckFinish();
  void AddAATPoint(int taskwaypoint);
  void CheckInSector();
  void TaskStatistics(const double this_maccready,
		      const double cruise_efficiency);
  void AATStats_Time();
  void AATStats_Distance();
  void AATStats();
  void CheckTransitionFinalGlide();
  void DebugTaskCalculations();
  void TaskSpeed(const double this_maccready,
		 const double cruise_efficiency);
  void LDNext(const double LegToGo);
  void CheckForceFinalGlide();
  double SpeedHeight();
  bool TaskAltitudeRequired(double this_maccready, double *Vfinal,
			    double *TotalTime, double *TotalDistance,
			    int *ifinal,
			    const double cruise_efficiency);
  double MacCreadyOrAvClimbRate(double this_maccready);
  void CheckFinalGlideThroughTerrain(double LegToGo, double LegBearing);
  // TODO: some of these can move into task class
  // abort stuff
  int CalculateWaypointApproxDistance(int scx_aircraft, 
				       int scy_aircraft,
				       int i);
  double CalculateWaypointArrivalAltitude(int i);
  // best alternate
  void AlertBestAlternate(short soundmode);
  void DoBestAlternateSlow();
  void DoAlternates(int AltWaypoint);
  void SearchBestAlternate();
protected:
  virtual void SaveTaskSpeed(double val) = 0;
  virtual void SetLegStart();
  virtual void ProcessIdle(const MapWindowProjection &map);
  double FAIFinishHeight(int wp) const;
public:
  virtual void ResetEnter();
  double AATCloseDistance(void) const {
    return max(100,Basic().Speed*1.5);
  }
};


class GlideComputer: public 
  GlideComputerAirData,
  GlideComputerTask,
  GlideComputerStats
{
public:
  GlideComputer();

  //protected:
  VegaVoice    vegavoice;
  void ResetFlight(const bool full=true);
protected:
  virtual void StartTask(const bool do_advance,
			 const bool do_announce);
  void DoLogging();
  virtual void SaveTaskSpeed(double val);
  virtual void SetLegStart();
  virtual void AnnounceWayPointSwitch(bool do_advance);

public:
  void Initialise();
  bool ProcessGPS(); // returns true if idle needs processing
  virtual void ProcessIdle(const MapWindowProjection &map);
  virtual bool ProcessVario();
  virtual bool InsideStartHeight(const DWORD Margin=0) const;
  virtual bool ValidStartSpeed(const DWORD Margin=0) const;
  virtual void IterateEffectiveMacCready();
  virtual void ResetEnter() {
    GlideComputerTask::ResetEnter();
  }

  // TODO: make these consts
  SnailTrail &GetSnailTrail() { return snail_trail; };
  OLCOptimizer &GetOLC() { return olc; };
  FlightStatistics &GetFlightStats() { return flightstats; };
private:
  void CalculateTeammateBearingRange();
  void CalculateOwnTeamCode();
};


double FAIFinishHeight(const SETTINGS_COMPUTER &settings,
		       const DERIVED_INFO& Calculated, int wp);

#endif

