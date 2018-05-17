/* Copyright_License {

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

#include "Math/FastMath.hpp"
#include "Computer/ThermalLocator.hpp"
#include "TestUtil.hpp"

static inline double
thermal_fn(int x)
{
  return exp((-0.2/ThermalLocator::TLOCATOR_NMAX)*pow((double)x, 1.5));
}

int main(int argc, char **argv)
{
  plan_tests(ThermalLocator::TLOCATOR_NMAX);

  for (unsigned i = 0; i < ThermalLocator::TLOCATOR_NMAX; ++i)
    ok1((int)(thermal_fn(i) * 1024 * 1024) ==
        (int)(thermal_recency_fn(i) * 1024 * 1024));

  return exit_status();
}
