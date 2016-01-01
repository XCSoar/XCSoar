/* Copyright_License {

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

#include "Flight.hpp"
#include "IGCFixEnhanced.hpp"
#include "DebugReplay.hpp"
#include "DebugReplayIGC.hpp"
#include "DouglasPeuckerMod.hpp"

#include <vector>

Flight::Flight(const char* _flight_file, bool _keep_flight)
  : fixes(nullptr), keep_flight(_keep_flight), flight_file(_flight_file) {
  if (keep_flight)
    ReadFlight();

  qnh = AtmosphericPressure::Standard();
  qnh_available.Clear();
}

void Flight::ReadFlight() {
  fixes = new std::vector<IGCFixEnhanced>;

  DebugReplay *replay = DebugReplayIGC::Create(Path(flight_file));

  if (replay) {
    if (qnh_available)
      replay->SetQNH(qnh);

    while (replay->Next()) {
      IGCFixEnhanced fix;
      fix.Clear();
      if (fix.Apply(replay->Basic(), replay->Calculated())) {
        fixes->push_back(fix);
      }
    }

    delete replay;
  }
}

void Flight::Reduce(const BrokenDateTime start, const BrokenDateTime end,
                    const unsigned num_levels, const unsigned zoom_factor,
                    const double threshold, const bool force_endpoints,
                    const unsigned max_delta_time, const unsigned max_points) {
  // we need the whole flight, so read it now...
  if (!keep_flight) {
    ReadFlight();
    keep_flight = true;
  }

  DouglasPeuckerMod dp(num_levels, zoom_factor, threshold,
    force_endpoints, max_delta_time, max_points);

  unsigned start_index = 0,
           end_index = 0;

  int64_t start_time = start.ToUnixTimeUTC(),
          end_time = end.ToUnixTimeUTC();

  for (auto fix : *fixes) {
    if (BrokenDateTime(fix.date, fix.time).ToUnixTimeUTC() < start_time)
      start_index++;

    if (BrokenDateTime(fix.date, fix.time).ToUnixTimeUTC() < end_time)
      end_index++;
    else
      break;
  }

  end_index = std::min(end_index, unsigned(fixes->size()));
  start_index = std::min(start_index, end_index);

  dp.Encode(*fixes, start_index, end_index);
}

Flight::~Flight() {
  if (keep_flight)
    delete fixes;
}

