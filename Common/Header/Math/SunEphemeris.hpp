/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#if !defined(SUN_EPHEMERIS_HPP)
#define SUN_EPHEMERIS_HPP

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

class SunEphemeris {

  double L,g,daylen;

  //   Get the days to J2000
  //   h is UT in decimal hours
  //   FNday only works between 1901 to 2099 - see Meeus chapter 7

  double FNday (int y, int m, int d, float h);

  //   the function below returns an angle in the range
  //   0 to 2*PI

  double FNrange (double x);

  // Calculating the hourangle
  //
  double f0(double lat, double declin);

  // Calculating the hourangle for twilight times
  //
  double f1(double lat, double declin);

  //   Find the ecliptic longitude of the Sun

  double FNsun (double d);

  // Display decimal hours in hours and minutes
  void showhrmn(double dhr);

 public:

  double twam,altmax,noont,settm,riset,twpm;

  int CalcSunTimes(float longit, float latit,
		   const NMEA_INFO &GPS_INFO,
		   const DERIVED_INFO &CALCULATED_INFO,
		   double tzone);
};


#endif
