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

#ifndef BASIC_COMPUTER_HPP
#define BASIC_COMPUTER_HPP

#include "GroundSpeedComputer.hpp"

struct MoreData;
struct DerivedInfo;
class AtmosphericPressure;
struct FeaturesSettings;
struct ComputerSettings;

/**
 * A computer which adds missing values to #NMEA_INFO.  It performs
 * simple and fast calculations after every GPS update, cheap enough
 * to run outside of the #CalculationThread.
 */
class BasicComputer {
  GroundSpeedComputer ground_speed;

public:
  /**
   * Fill the missing attributes with a fallback.
   */
  void Fill(MoreData &data, const AtmosphericPressure qnh,
            const FeaturesSettings &features);

  void Fill(MoreData &data, const ComputerSettings &settings_computer);

  /**
   * Runs all calculations.
   *
   * @param data the current sensor data structure
   * @param last the previous sensor data structure
   * @param last_gps the previous GPS fix
   * @param calculations the most up-to-date version of calculated values
   */
  void Compute(MoreData &data, const MoreData &last, const MoreData &last_gps,
               const DerivedInfo &calculated);
};

#endif
