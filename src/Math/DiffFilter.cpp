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

#include "DiffFilter.hpp"
#include "Constants.hpp"

void
DiffFilter::Reset(const double x0, const double y0)
{
  for (unsigned i = 0; i < x.size(); i++)
    x[i] = x0 - y0 * i;
}

double
DiffFilter::Update(const double x0)
{
  std::copy_backward(x.cbegin(), std::prev(x.cend()), x.end());
  x.front() = x0;

  /// @note not sure why need to divide by pi/2 here
  return ((x.back() - x.front()) / 16 + x[2] - x[4]) / M_PI_2;
}

