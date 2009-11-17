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

#include "Math/Pressure.h"

#include <math.h>

double QNH = (double)1013.2; /**< Pressure on sea level */
double QFEAltitudeOffset= 0.0; /**< not in use(?) */

/**
 * Converts a QNH-based altitude to the corresponding pressure
 *
 * See http://wahiduddin.net/calc/density_altitude.htm
 *
 * Example:
 * alt= 100, QNH=1014 => ps = 100203 Pa
 * @see StaticPressureToAltitude
 * @param alt Altitude over QNH-based zero
 * @return Air pressure at given altitude
 */
double QNHAltitudeToStaticPressure(double alt) {
  const double k1=0.190263;
  const double k2=8.417286e-5;
  return 100.0*pow((pow(QNH,k1)-k2*alt),1.0/k1);
}

/**
 * Converts a pressure value to the corresponding QNH-based altitude
 *
 * See http://wahiduddin.net/calc/density_altitude.htm
 *
 * Example:
 * QNH=1014, ps=100203 => alt = 100
 * @see QNHAltitudeToStaticPressure
 * @param ps Air pressure
 * @return Altitude over QNH-based zero
 */
double StaticPressureToAltitude(double ps) {
  const double k1=0.190263;
  const double k2=8.417286e-5;
  return (pow(QNH,k1) - pow(ps/100.0, k1))/k2;
}
/**
 * Converts altitude with QNH=1013.25 reference to QNH adjusted altitude
 * @param alt 1013.25-based altitude
 * @return QNH-based altitude
 */
double AltitudeToQNHAltitude(double alt) {
  const double k1=0.190263;
  double ps = pow((44330.8-alt)/4946.54,1.0/k1);
  return StaticPressureToAltitude(ps);
}

/**
 * Calculates the current QNH by comparing a raw altitude
 * to a known altitude of a certain location
 *
 * Original comment:
 *
 * find QNH so that the static pressure gives the specified altitude
 * (altitude can come from GPS or known airfield altitude or terrain
 * height on ground)
 *
 * This function assumes the barometric altitude (alt_raw) is
 * already adjusted for QNH ---> the function returns the
 * QNH value to make the barometric altitude equal to the
 * alt_known value.
 * @param alt_raw Current altitude
 * @param alt_known Altitude of a known location
 * @return QNH
 */
double FindQNH(double alt_raw, double alt_known) {
  const double k1=0.190263;
  const double k2=8.417286e-5;

  // step 1, find static pressure from device assuming it's QNH adjusted
  double psraw = QNHAltitudeToStaticPressure(alt_raw);
  // step 2, calculate QNH so that reported alt will be known alt
  return pow(pow(psraw/100.0,k1) + k2*alt_known,1/k1);

  // example, QNH=1014, ps=100203
  // alt= 100
  // alt_known = 120
  // qnh= 1016
}

/**
 * Calculates the air density from a given QNH-based(?) altitude
 * @param altitude QNH-based(?) altitude
 * @return Air density
 */
double AirDensity(double altitude) {
  double rho = pow((44330.8-altitude)/42266.5,1.0/0.234969);
  return rho;
}

/**
 * Divide TAS by this number to get IAS
 * @param altitude QNH-based(?) altitude
 * @return Air density ratio(?)
 */
double AirDensityRatio(double altitude) {
  double rho = AirDensity(altitude);
  double rho_rat = sqrt(1.225/rho);
  return rho_rat;
}
