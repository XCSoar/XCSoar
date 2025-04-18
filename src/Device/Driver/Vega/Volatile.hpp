// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

    void SendTo(Port &port, OperationEnvironment &env) const;
  };
}
