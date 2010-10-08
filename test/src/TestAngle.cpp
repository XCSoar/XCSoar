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

#include "Math/Angle.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  plan_tests(38);

  ok1(equals(Angle::degrees(fixed_90).magnitude_degrees(), 90));
  ok1(equals(Angle::degrees(-fixed_90).magnitude_degrees(), 90));

  ok1(equals(Angle::degrees(fixed_90).Reciprocal(), 270));
  ok1(equals(Angle::degrees(fixed_270).Reciprocal(), 90));

  ok1(equals(Angle::degrees(fixed(361)).as_bearing(), 1));
  ok1(equals(Angle::degrees(fixed(180)).as_bearing(), 180));
  ok1(equals(Angle::degrees(fixed(-180)).as_bearing(), 180));
  ok1(equals(Angle::degrees(fixed(-270)).as_bearing(), 90));

  ok1(equals(Angle::degrees(fixed_90).as_delta(), 90));
  ok1(equals(Angle::degrees(fixed(179)).as_delta(), 179));
  ok1(equals(Angle::degrees(fixed(-179)).as_delta(), -179));
  ok1(equals(Angle::degrees(fixed_270).as_delta(), -90));

  ok1(Angle::degrees(fixed_90).between(Angle::degrees(fixed_zero),
                                          Angle::degrees(fixed_180)));

  ok1(!Angle::degrees(fixed_90).between(Angle::degrees(fixed_180),
                                           Angle::degrees(fixed_zero)));

  ok1(Angle::degrees(fixed_zero).between(Angle::degrees(fixed_270),
                                            Angle::degrees(fixed_90)));

  ok1(equals(Angle::degrees(fixed_zero).sin(), 0));
  ok1(equals(Angle::degrees(fixed_90).sin(), 1));
  ok1(equals(Angle::degrees(fixed_180).sin(), 0));
  ok1(equals(Angle::degrees(fixed_270).sin(), -1));
  ok1(equals(Angle::degrees(fixed_360).sin(), 0));

  ok1(equals(Angle::degrees(fixed_zero).cos(), 1));
  ok1(equals(Angle::degrees(fixed_90).cos(), 0));
  ok1(equals(Angle::degrees(fixed_180).cos(), -1));
  ok1(equals(Angle::degrees(fixed_270).cos(), 0));
  ok1(equals(Angle::degrees(fixed_360).cos(), 1));

  ok1(equals(Angle::degrees(fixed_zero).fastsine(), 0));
  ok1(equals(Angle::degrees(fixed_90).fastsine(), 1));
  ok1(equals(Angle::degrees(fixed_180).fastsine(), 0));
  ok1(equals(Angle::degrees(fixed_270).fastsine(), -1));
  ok1(equals(Angle::degrees(fixed_360).fastsine(), 0));

  ok1(equals(Angle::degrees(fixed_zero).fastcosine(), 1));
  ok1(equals(Angle::degrees(fixed_90).fastcosine(), 0));
  ok1(equals(Angle::degrees(fixed_180).fastcosine(), -1));
  ok1(equals(Angle::degrees(fixed_270).fastcosine(), 0));
  ok1(equals(Angle::degrees(fixed_360).fastcosine(), 1));

  ok1(equals(Angle::degrees(fixed_90).BiSector(Angle::degrees(fixed_180)),
                45));
  ok1(equals(Angle::degrees(fixed_270).BiSector(Angle::degrees(fixed_zero)),
                225));
  ok1(equals(Angle::degrees(fixed_270).BiSector(Angle::degrees(fixed_180)),
                315));

  return 0;
}
