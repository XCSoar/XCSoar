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

#include "Units.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(10);

  Units::SetUserDistanceUnit(unMeter);
  ok1(equals(Units::ToUserDistance(fixed_one), 1));

  Units::SetUserDistanceUnit(unKiloMeter);
  ok1(equals(Units::ToUserDistance(fixed_one), 0.001));

  Units::SetUserTemperatureUnit(unKelvin);
  ok1(equals(Units::ToUserTemperature(fixed_zero), fixed_zero));
  ok1(equals(Units::ToSysTemperature(fixed_zero), fixed_zero));

  Units::SetUserTemperatureUnit(unGradCelcius);
  ok1(equals(Units::ToUserTemperature(fixed_zero), -273.15));
  ok1(equals(Units::ToUserTemperature(fixed(20)), -253.15));
  ok1(equals(Units::ToSysTemperature(fixed_zero), 273.15));
  ok1(equals(Units::ToSysTemperature(fixed(20)), 293.15));

  Units::SetUserTemperatureUnit(unGradFahrenheit);
  ok1(equals(Units::ToUserTemperature(fixed_zero), -459.67));
  ok1(equals(Units::ToSysTemperature(fixed_zero), 255.37));

  return 0;
}
