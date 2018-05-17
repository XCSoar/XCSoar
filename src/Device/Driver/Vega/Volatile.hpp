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

#ifndef XCSOAR_VEGA_VOLATILE_HPP
#define XCSOAR_VEGA_VOLATILE_HPP

struct DerivedInfo;
class Port;
class OperationEnvironment;

namespace Vega {
  struct VolatileData {
    /**
     * MacCready setting [cm/s].
     */
    unsigned mc;

    /**
     * QNH [0.1 hPa].
     */
    unsigned qnh;

    /**
     * Terrain altitude [m].
     */
    int terrain_altitude;

    /**
     * Speed to fly [cm/s].
     */
    unsigned stf;

    bool circling;

    constexpr VolatileData()
      :mc(0), qnh(10133), terrain_altitude(0), stf(0), circling(false) {}

    void CopyFrom(const DerivedInfo &calculated);

    bool SendTo(Port &port, OperationEnvironment &env) const;
  };
}

#endif
