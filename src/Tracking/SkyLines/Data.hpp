/*
Copyright_License {

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

#ifndef XCSOAR_TRACKING_SKYLINES_TRAFFIC_HPP
#define XCSOAR_TRACKING_SKYLINES_TRAFFIC_HPP

#include "Geo/GeoPoint.hpp"
#include "Thread/Mutex.hpp"
#include "Util/tstring.hpp"
#include "Compiler.h"

#include <map>

#include <stdint.h>

namespace SkyLinesTracking {
  struct Data {
    struct Traffic {
      GeoPoint location;
      int altitude;

      Traffic() = default;
      constexpr Traffic(GeoPoint _location,
                        int _altitude)
        :location(_location), altitude(_altitude) {}
    };

    mutable Mutex mutex;

    std::map<uint32_t, Traffic> traffic;

    /**
     * A database of user-id to display-name.  An empty string means
     * the server has failed/refused to supply a name.
     */
    std::map<uint32_t, tstring> user_names;

    gcc_pure
    bool IsUserKnown(uint32_t id) const {
      return user_names.find(id) != user_names.end();
    }
  };
}

#endif
