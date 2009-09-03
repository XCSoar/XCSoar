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

#include "XCSoar.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "FlightStatistics.hpp"
#include "AATDistance.h"
#include "OnLineContest.h"
#include "Audio/VegaVoice.h"
#include "GlideRatio.hpp"
#include "ThermalLocator.h"
#include "windanalyser.h"
#include "SnailTrail.hpp"

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputerBlackboard {
public:
  NMEA_INFO     gps_info;
  DERIVED_INFO  calculated_info;
protected:
  void ResetFlight(const bool full=true);
  void StartTask();
  void Initialise();
  void SaveFinish();


  virtual double GetAverageThermal();
private:
  DERIVED_INFO Finish_Derived_Info;
};

class GlideComputerAirData: virtual public GlideComputerBlackboard {
public:
  GlideComputerAirData();
  ldrotary_s           rotaryLD;
  ThermalLocator thermallocator;
  WindAnalyser   windanalyser;
protected:

  void ResetFlight(const bool full=true);
  void Initialise();
  void ProcessVertical();
  void ProcessBasic();
  ///

  void DoWindCirclingMode(const bool left);
  void DoWindCirclingSample();
  void DoWindCirclingAltitude();
  void SetWindEstimate(const double wind_speed,
		       const double wind_bearing,
		       const int quality);
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
  virtual double GetAverageThermal();
protected:
  virtual void SaveTaskSpeed(double val);
};

class GlideComputerTask: virtual public GlideComputerBlackboard {
public:
  AATDistance          aatdistance;
  OLCOptimizer         olc;
  // CalculationsAutoMc
  static void DoAutoMacCready(double mc_setting);
protected:
  void ProcessBasicTask(const double mc_setting, 
			const double cruise_efficiency);
  void ResetFlight(const bool full=true);
  virtual void StartTask(const bool do_advance,
			 const bool do_announce);
  virtual void AnnounceWayPointSwitch(bool do_advance);
  bool DoLogging();
private:
  void DistanceToHome();
  void DistanceToNext();
  void AltitudeRequired(const double mc_setting,
			const double cruise_efficiency);
  double AATCloseBearing();
  double FAIFinishHeight(int wp);
  bool InsideStartHeight(const DWORD Margin=0);
  bool ValidStartSpeed(const DWORD Margin=0);
  bool InTurnSector(const int the_turnpoint);
  bool InFinishSector(const int i);
  bool ValidFinish();
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
  void InSector();
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
  // TODO: some of these can move into task class
protected:
  virtual void SaveTaskSpeed(double val);
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
public:
  void Initialise();
  void ProcessGPS();
  void ProcessVario();
};

#endif

