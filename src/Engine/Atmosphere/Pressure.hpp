/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_ATMOSPHERE_PRESSURE_H
#define XCSOAR_ATMOSPHERE_PRESSURE_H

#include "Math/fixed.hpp"
#include "Compiler.h"

/**
 * ICAO Standard Atmosphere calculations (valid in Troposphere, alt<11000m)
 *
 */
class AtmosphericPressure 
{
  /** Pressure at sea level, hPa */
  fixed qnh;

public:
  static gcc_constexpr_data fixed standard = fixed(1013.25);

  /**
   * Non-initialising constructor.
   */
  AtmosphericPressure() = default;

  /**
   * @param qnh the QNH in hPa
   */
  gcc_constexpr_ctor
  AtmosphericPressure(fixed _qnh):qnh(_qnh) {}

  /**
   * Configure the standard pressure (1013.25 hPa);
   */
  void SetStandardPressure() {
    SetQNH(standard);
  }

  /**
   * Set QNH value (Mean Sea Level pressure)
   *
   * @param set New value of QNH (hPa)
   */
  void SetQNH(const fixed _qnh) {
    qnh = _qnh;
  }

  /**
   * Access QNH value
   *
   * @return QNH value (hPa)
   */
  fixed GetQNH() const {
    return qnh;
  }

  /**
   * Calculates the current QNH by comparing a raw altitude
   * to a known altitude of a certain location
   *
   * This function assumes the altitude (alt_raw) is
   * a pressure altitude based on QNH=1013.25 hPa
   * ---> the function returns the QNH value to make
   * the barometric altitude equal to the alt_known value.
   * @param alt_raw Current pressure altitude (m)
   * @param alt_known Altitude of a known location (m)
   */
  gcc_pure
  fixed FindQNHFromPressureAltitude(const fixed alt_raw,
                                    const fixed alt_known) const;

  /**
   * Converts altitude with QNH=1013.25 reference to QNH adjusted altitude
   * @param alt 1013.25-based altitude (m)
   * @return QNH-based altitude (m)
   */
  gcc_pure
  fixed PressureAltitudeToQNHAltitude(const fixed alt) const;

  /**
   * Converts QNH adjusted altitude to pressure altitude (with QNH=1013.25 as reference)
   * @param alt QNH-based altitude(m)
   * @return pressure altitude (m)
   */
  gcc_pure
  fixed QNHAltitudeToPressureAltitude(const fixed alt) const;

  /**
   * Calculates the air density from a given QNH-based altitude
   * @param altitude QNH-based altitude (m)
   * @return Air density (kg/m^3)
   */
  gcc_pure
  static fixed AirDensity(const fixed altitude);

  /**
   * Divide TAS by this number to get IAS
   * @param altitude QNH-based altitude (m)
   * @return Ratio of TAS to IAS
   */
  gcc_pure
  static fixed AirDensityRatio(const fixed altitude);

  /**
   * Converts a pressure value to the corresponding QNH-based altitude
   *
   * See http://wahiduddin.net/calc/density_altitude.htm
   *
   * Example:
   * QNH=1014, ps=100203 => alt = 100
   * @see QNHAltitudeToStaticPressure
   * @param ps Air pressure (Pa)
   * @return Altitude over QNH-based zero (m)
   */
  gcc_pure
  fixed StaticPressureToQNHAltitude(const fixed ps) const;

  /**
   * Converts a QNH-based altitude to the corresponding pressure
   *
   * See http://wahiduddin.net/calc/density_altitude.htm
   *
   * Example:
   * alt= 100, QNH=1014 => ps = 100203 Pa
   * @see StaticPressureToAltitude
   * @param alt Altitude over QNH-based zero (m)
   * @return Air pressure at given altitude (Pa)
   */
  gcc_pure
  fixed QNHAltitudeToStaticPressure(const fixed alt) const;

  /**
   * Converts a pressure value to pressure altitude (with QNH=1013.25 as reference)
   * @param ps Air pressure (Pa)
   * @return pressure altitude (m)
   */
  gcc_const
  static fixed StaticPressureToPressureAltitude(const fixed ps);

  /**
   * Converts a 1013.25 hPa based altitude to the corresponding pressure
   *
   * @see StaticPressureToAltitude
   * @param alt Altitude over 1013.25 hPa based zero(m)
   * @return Air pressure at given altitude (Pa)
   */
  gcc_const
  static fixed PressureAltitudeToStaticPressure(const fixed alt);
};

#endif
