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

#ifndef XCSOAR_WIND_COMPUTER_HPP
#define XCSOAR_WIND_COMPUTER_HPP

#include "CirclingWind.hpp"
#include "WindEKFGlue.hpp"
#include "Store.hpp"

struct WindSettings;
class GlidePolar;
struct NMEAInfo;
struct MoreData;
struct DerivedInfo;

/**
 * Calculate the wind vector.
 *
 * Dependencies: #FlyingComputer, #CirclingComputer.
 */
class WindComputer {
  CirclingWind circling_wind;
  WindEKFGlue wind_ekf;

  // TODO: protect with a Mutex
  WindStore wind_store;

  /**
   * The EKF algorithm is available, which skips WindStore and
   * CirclingWind.
   */
  bool ekf_active;

public:
  const WindStore &GetWindStore() const {
    return wind_store;
  }

  void Reset();

  void Compute(const WindSettings &settings,
               const GlidePolar &glide_polar,
               const MoreData &basic, DerivedInfo &calculated);

  void ComputeHeadWind(const NMEAInfo &basic, DerivedInfo &calculated);

  /**
   * Select one of the wind values and write it into
   * DerivedInfo::wind, according to the configuration.
   */
  void Select(const WindSettings &settings,
              const NMEAInfo &basic, DerivedInfo &calculated);
};

#endif
