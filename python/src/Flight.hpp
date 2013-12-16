/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef PYTHON_FLIGHT_HPP
#define PYTHON_FLIGHT_HPP

#include "IGCFixEnhanced.hpp"
#include "DebugReplayIGC.hpp"
#include "DebugReplayVector.hpp"
#include "FlightTimes.hpp"

#include <vector>

class DebugReplay;

class Flight {
private:
  std::vector<IGCFixEnhanced> *fixes;
  bool keep_flight;
  const char *flight_file;

public:
  Flight(const char* _flight_file, bool _keep_flight);
  ~Flight();

  /**
   * Return a DebugReplay, either direct from file or from memory,
   * depending on the keep_flight flag. Don't forget to delete
   * the replay after use.
   */
  DebugReplay *Replay() {
    if (keep_flight) return DebugReplayVector::Create(*fixes);
    else return DebugReplayIGC::Create(flight_file);
  };

  /* Search for flights within the fixes */
  unsigned Times(std::vector<FlightTimeResult> &results) {
    DebugReplay *replay = Replay();
    FlightTimes(*replay, results);
    delete replay;

    return results.size();
  };

private:
  /* Read the flight into memory */
  void ReadFlight();
};

#endif /* PYTHON_FLIGHT_HPP */
