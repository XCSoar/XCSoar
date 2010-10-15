/* Copyright_License {

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

#include "Math/SunEphemeris.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "DateTime.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(5);

  const GeoPoint location(Angle::degrees(fixed(7.7061111111111114)),
                          Angle::degrees(fixed(51.051944444444445)));
  BrokenDateTime dt;
  dt.year = 2010;
  dt.month = 9;
  dt.day = 24;
  dt.hour = 8;
  dt.minute = 21;
  dt.second = 12;

  SunEphemeris sun;
  sun.CalcSunTimes(location, dt, fixed_two);

  ok1(between(sun.MorningTwilight, 6.88, 6.9));
  ok1(between(sun.TimeOfNoon, 13.3, 13.4));
  ok1(between(sun.TimeOfSunSet, 19.36, 19.40));
  ok1(between(sun.TimeOfSunRise, 7.32, 7.41));
  ok1(between(sun.EveningTwilight, 19.81, 19.82));

  return 0;
}
