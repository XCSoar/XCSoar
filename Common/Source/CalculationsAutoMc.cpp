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

#include "Calculations.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "SettingsTask.hpp"
#include "Device/device.h"
#include "McReady.h"
#include "Math/LowPassFilter.hpp"
#include "Logger.h"
#include "GlideSolvers.hpp"
#include <math.h>

int  AutoMcMode = 0;
// 0: Final glide only
// 1: Set to average if in climb mode
// 2: Average if in climb mode, final glide in final glide mode


void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  bool is_final_glide = false;

  if (!Calculated->AutoMacCready) return;

  //  LockFlightData();
  LockTaskData();

  double mc_new = MACCREADY;
  static bool first_mc = true;

  if (Calculated->FinalGlide && ActiveIsFinalWaypoint()) {
    is_final_glide = true;
  } else {
    first_mc = true;
  }

  double av_thermal = -1;
  if (flightstats.ThermalAverage.y_ave>0) {
    if (Calculated->Circling && (Calculated->AverageThermal>0)) {
      av_thermal = (flightstats.ThermalAverage.y_ave
		*flightstats.ThermalAverage.sum_n
		+ Calculated->AverageThermal)/
	(flightstats.ThermalAverage.sum_n+1);
    } else {
      av_thermal = flightstats.ThermalAverage.y_ave;
    }
  } else if (Calculated->Circling && (Calculated->AverageThermal>0)) {
    // insufficient stats, so use this/last thermal's average
    av_thermal = Calculated->AverageThermal;
  }

  if (!ValidTaskPoint(ActiveWayPoint)) {
    if (av_thermal>0) {
      mc_new = av_thermal;
    }
  } else if ( ((AutoMcMode==0)||(AutoMcMode==2)) && is_final_glide) {

    double time_remaining = Basic->Time-Calculated->TaskStartTime-9000;
    if (EnableOLC
	&& (OLCRules==0)
	&& (Calculated->NavAltitude>Calculated->TaskStartAltitude)
	&& (time_remaining>0)) {

      mc_new = MacCreadyTimeLimit(Basic, Calculated,
				  Calculated->WaypointBearing,
				  time_remaining,
				  Calculated->TaskStartAltitude);

    } else if (Calculated->TaskAltitudeDifference0>0) {

      // only change if above final glide with zero Mc
      // otherwise when we are well below, it will wind Mc back to
      // zero

      double slope =
	(Calculated->NavAltitude + Calculated->EnergyHeight
	 - FAIFinishHeight(Basic, Calculated, ActiveWayPoint))/
	(Calculated->WaypointDistance+1);

      double mc_pirker = PirkerAnalysis(Basic, Calculated,
					Calculated->WaypointBearing,
					slope);
      mc_pirker = max(0.0, mc_pirker);
      if (first_mc) {
	// don't allow Mc to wind down to zero when first achieving
	// final glide; but do allow it to wind down after that
	if (mc_pirker >= mc_new) {
	  mc_new = mc_pirker;
	  first_mc = false;
	} else if (AutoMcMode==2) {
	  // revert to averager based auto Mc
	  if (av_thermal>0) {
	    mc_new = av_thermal;
	  }
	}
      } else {
	mc_new = mc_pirker;
      }
    } else { // below final glide at zero Mc, never achieved final glide
      if (first_mc && (AutoMcMode==2)) {
	// revert to averager based auto Mc
	if (av_thermal>0) {
	  mc_new = av_thermal;
	}
      }
    }
  } else if ( (AutoMcMode==1) || ((AutoMcMode==2)&& !is_final_glide) ) {
    if (av_thermal>0) {
      mc_new = av_thermal;
    }
  }

  MACCREADY = LowPassFilter(MACCREADY,mc_new,0.15);

  UnlockTaskData();
  //  UnlockFlightData();

}
