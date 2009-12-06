/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

/**
 * ICAO Standard Atmosphere calculations (valid in Troposphere, alt<11000m)
 *
 */
class AtmosphericPressure 
{
public:

/** 
 * Default constructor, sets QNH to International Standard Atmosphere
 * (1013.25)
 */
  AtmosphericPressure();

/** 
 * Set QNH value (Mean Sea Level pressure)
 * 
 * @param set New value of QNH (hPa)
 */
  void set_QNH(const fixed set) {
    m_QNH = set;
  }

/** 
 * Access QNH value
 * 
 * @return QNH value (hPa)
 */
  fixed get_QNH() const {
    return m_QNH;
  }

/**
 * Sets the current QNH by comparing a raw altitude
 * to a known altitude of a certain location
 *
 * This function assumes the barometric altitude (alt_raw) is
 * already adjusted for QNH ---> the function returns the
 * QNH value to make the barometric altitude equal to the
 * alt_known value.
 * @param alt_raw Current pressure altitude (m)
 * @param alt_known Altitude of a known location (m)
 */
  void FindQNH(const fixed alt_raw, const fixed alt_known);

/**
 * Converts altitude with QNH=1013.25 reference to QNH adjusted altitude
 * @param alt 1013.25-based altitude (m)
 * @return QNH-based altitude (m)
 */
  fixed AltitudeToQNHAltitude(const fixed alt) const;

/**
 * Calculates the air density from a given QNH-based altitude
 * @param altitude QNH-based altitude (m)
 * @return Air density (kg/m^3)
 */
  fixed AirDensity(const fixed altitude) const;

/**
 * Divide TAS by this number to get IAS
 * @param altitude QNH-based altitude (m)
 * @return Ratio of TAS to IAS
 */
  fixed AirDensityRatio(const fixed altitude) const;

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
  fixed QNHAltitudeToStaticPressure(const fixed alt) const;

private:

  fixed m_QNH; /**< Pressure at sea level, hPa */
};

#endif
