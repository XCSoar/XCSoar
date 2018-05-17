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

#include "AirDensity.hpp"

#include <math.h>

#define isa_sea_level_density 1.225
#define k4 44330.8
#define k6 1.0 / 42266.5
#define k7 1.0 / 0.234969

double
AirDensity(const double altitude)
{
  return pow((k4 - altitude) * k6, k7);
}

double
AirDensityRatio(const double altitude)
{
  return sqrt(isa_sea_level_density / AirDensity(altitude));
}
