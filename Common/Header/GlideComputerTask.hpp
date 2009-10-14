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

#if !defined(XCSOAR_GLIDECOMPUTER_TASK_HPP)
#define XCSOAR_GLIDECOMPUTER_TASK_HPP

#include "GlideComputerBlackboard.hpp"
#include "AATDistance.h"
#include "OnLineContest.h"
#include "GPSClock.hpp"

struct WAYPOINT;
struct WPCALC;

class GlideComputerTask: virtual public GlideComputerBlackboard {
public:
  GlideComputerTask(): olc_clock(5.0) {};
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
  // stuff for display
  void CalculateWaypointReachable(void);
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
  bool InStartSector(bool *CrossedStart);
  bool ReadyToStart();
  bool ReadyToAdvance(bool reset=true, bool restart=false);
  void CheckStart();
  void CheckRestart();
  void CheckFinish();
  void AddAATPoint(const unsigned taskwaypoint);
  void CheckInSector();
  void TaskStatistics(const double this_maccready,
		      const double cruise_efficiency);
  void LegSpeed();
  void TerrainWarning();
  void DistanceCovered();
  void AATStats_Time();
  void AATStats_Distance();
  void AATStats();
  void CheckTransitionFinalGlide();
  void DebugTaskCalculations();
  void TaskSpeed(const double this_maccready,
		 const double cruise_efficiency);
  void LDNext();
  void CheckForceFinalGlide();
  double SpeedHeight();
  bool TaskAltitudeRequired(double this_maccready, double *Vfinal,
			    double *TotalTime, double *TotalDistance,
			    const double cruise_efficiency);
  double MacCreadyOrAvClimbRate(double this_maccready);
  void CheckFinalGlideThroughTerrain(double LegToGo, double LegBearing);
  // TODO: some of these can move into task class
  // abort stuff
  int CalculateWaypointApproxDistance(const POINT &screen,
                                      const WAYPOINT &way_point);
  double CalculateWaypointArrivalAltitude(const WAYPOINT &way_point,
                                          WPCALC &calc);
  // best alternate
  void AlertBestAlternate(short soundmode);
  void DoBestAlternateSlow();
  void DoAlternates(int AltWaypoint);
  void SearchBestAlternate();
protected:

  virtual void SaveTaskSpeed(double val) = 0;
  virtual void SetLegStart();
  virtual void ProcessIdle();
  double FAIFinishHeight(int wp) const;
private:
  GPSClock olc_clock;
public:
  virtual void ResetEnter();
  double AATCloseDistance(void) const {
    return max(100,Basic().Speed*1.5);
  }
};

double FAIFinishHeight(const SETTINGS_COMPUTER &settings,
		       const DERIVED_INFO& Calculated, int wp);

#endif

