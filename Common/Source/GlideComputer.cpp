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

#include "GlideComputer.hpp"
#include "McReady.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Persist.hpp"

GlideComputer::GlideComputer()
{

}


void GlideComputer::ResetFlight(const bool full)
{
  ScopeLock protect(mutexGlideComputer);

  GlideComputerBlackboard::ResetFlight(full);
  GlideComputerAirData::ResetFlight(full);
  GlideComputerTask::ResetFlight(full);
  GlideComputerStats::ResetFlight(full);
}


void GlideComputer::StartTask(const bool do_advance,
			      const bool do_announce) {

  //  GlideComputerBlackboard::StartTask();
  GlideComputerStats::StartTask();

  if (do_announce) {
    AnnounceWayPointSwitch(do_advance);
  } else {
    GlideComputerTask::StartTask(do_advance, do_announce);
  }
}

void GlideComputer::Initialise()
{
  ScopeLock protect(mutexGlideComputer);

  GlideComputerBlackboard::Initialise();
  GlideComputerAirData::Initialise();
  GlideComputerStats::Initialise();
  ResetFlight(true);

  LoadCalculationsPersist(&calculated_info);
  DeleteCalculationsPersist();
  // required to allow fail-safe operation
  // if the persistent file is corrupt and causes a crash

  ResetFlight(false);

}


void GlideComputer::DoLogging()
{
  if (GlideComputerStats::DoLogging()) {
    GlideComputerTask::DoLogging();
  }
}


void GlideComputer::ProcessGPS()
{
  double mc = GlidePolar::GetMacCready();
  double ce = GlidePolar::GetCruiseEfficiency();

  ProcessBasic();
  ProcessBasicTask(mc, ce);
  ProcessVertical();
}


void GlideComputer::ProcessVario()
{

}


void GlideComputer::SaveTaskSpeed(double val)
{
  GlideComputerStats::SaveTaskSpeed(val);
}
