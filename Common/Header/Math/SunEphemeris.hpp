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

struct NMEA_INFO;
struct DERIVED_INFO;
struct GEOPOINT;

class SunEphemeris {
  double L,g,daylen;

  double FNday (int y, int m, int d, float h);
  double FNrange (double x);
  double f0(double lat, double declin);
  double f1(double lat, double declin);
  double FNsun (double d);
  void showhrmn(double dhr);

 public:
  double twam, altmax, noont, settm, riset, twpm;

  int CalcSunTimes(const GEOPOINT &location, const NMEA_INFO &GPS_INFO,
      const DERIVED_INFO &CALCULATED_INFO, double tzone);
};

#endif
