/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "GlideComputerBlackboard.hpp"
#include "Audio/VegaVoice.h"
#include "GPSClock.hpp"
#include "GlideComputerAirData.hpp"
#include "GlideComputerStats.hpp"
#include "GlideComputerTask.hpp"

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputer:
    public GlideComputerAirData, GlideComputerTask, GlideComputerStats
{
public:
  GlideComputer(TaskManager& task,
                AirspaceWarningManager& as_manager,
                Airspaces& _airspaces);

  void ResetFlight(const bool full=true);
  void Initialise();
  bool ProcessGPS(); // returns true if idle needs processing
  virtual void ProcessIdle();
  virtual bool InsideStartHeight(const DWORD Margin=0) const;
  virtual bool ValidStartSpeed(const DWORD Margin=0) const;
  virtual void ResetEnter() {
    GlideComputerTask::ResetEnter();
  }

  // TODO: make these const
  /** Returns the FlightStatistics object */
  FlightStatistics &GetFlightStats() { return flightstats; }

protected:
  VegaVoice    vegavoice;
  virtual void StartTask(const bool do_advance, const bool do_announce);
  void DoLogging();
  virtual void SaveTaskSpeed(double val);
  virtual void SetLegStart();
  virtual void AnnounceWayPointSwitch(bool do_advance);
  virtual void OnTakeoff();
  virtual void OnLanding();
  virtual void OnSwitchClimbMode(bool isclimb, bool left);
  virtual void OnDepartedThermal();

private:
  void CalculateTeammateBearingRange();
  void CalculateOwnTeamCode();
  void FLARM_ScanTraffic();
};

#endif
