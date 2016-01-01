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

#include "Events.hpp"
#include "Input/InputQueue.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Settings.hpp"

void
GlideComputerEvents::Reset()
{
  last_flying = false;
  last_circling = false;
  last_final_glide = false;
  last_traffic = 0;
  last_new_traffic.Clear();
  last_teammate_in_sector = false;
}

void
GlideComputerEvents::OnCalculatedUpdate(const MoreData &basic,
                                        const DerivedInfo &calculated)
{
  /* check for take-off and landing */

  if (calculated.flight.flying != last_flying) {
    last_flying = calculated.flight.flying;
    if (calculated.flight.flying)
      InputEvents::processGlideComputer(GCE_TAKEOFF);
    else
      InputEvents::processGlideComputer(GCE_LANDING);
  }

  /* check the climb state */

  if (calculated.circling != last_circling) {
    last_circling = calculated.circling;
    if (calculated.circling)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CLIMB);
    else
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
  }

  /* check for new traffic */

  const FlarmData &flarm = basic.flarm;
  if (flarm.status.available) {
    if (flarm.status.rx > 0 && last_traffic == 0)
      // traffic has appeared..
      InputEvents::processGlideComputer(GCE_FLARM_TRAFFIC);
    else if (flarm.status.rx == 0 && last_traffic > 0)
      // traffic has disappeared..
      InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);
    last_traffic = flarm.status.rx;

    if (flarm.traffic.new_traffic.Modified(last_new_traffic)) {
      // new traffic has appeared
      last_new_traffic = flarm.traffic.new_traffic;
      InputEvents::processGlideComputer(GCE_FLARM_NEWTRAFFIC);
    }
  } else
    last_traffic = 0;

  /* check team mate */

  if (enable_team) {
    // Hysteresis for GlideComputerEvent
    // If (closer than 100m to the teammates last position and "event" not reset)
    if (calculated.teammate_vector.distance < 100 &&
        !last_teammate_in_sector) {
      last_teammate_in_sector = true;
      // Raise GCE_TEAM_POS_REACHED event
      InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
    } else if (calculated.teammate_vector.distance > 300) {
      // Reset "event" when distance is greater than 300m again
      last_teammate_in_sector = false;
    }
  }

  /* check for final glide */

  const bool final_glide = calculated.task_stats.flight_mode_final_glide;
  if (final_glide != last_final_glide) {
    last_final_glide = final_glide;

    InputEvents::processGlideComputer(final_glide
                                      ? GCE_FLIGHTMODE_FINALGLIDE
                                      : GCE_FLIGHTMODE_CRUISE);
  }
}

void
GlideComputerEvents::OnComputerSettingsUpdate(const ComputerSettings &settings)
{
  enable_team = settings.team_code.team_flarm_id.IsDefined() ||
    settings.team_code.team_code.IsDefined();
}
