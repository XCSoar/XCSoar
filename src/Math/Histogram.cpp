/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Math/Histogram.hpp"

#define NUM_SLOTS 60
#define SPREAD 0.15

void
Histogram::IncrementSlot(const unsigned i, const double mag)
{
  slots[i].y += mag;
  y_max = std::max(slots[i].y, y_max);
}

void
Histogram::UpdateHistogram(double x)
{
  assert(sum_n);

  unsigned i = (int)(m*(x-b)+0.5);
  if (i>= sum_n) i = sum_n-1;

  double mag = 1;

  if (i>0) {
    IncrementSlot(i-1, SPREAD);
    mag -= SPREAD;
  }
  if (i< sum_n-1) {
    IncrementSlot(i+1, SPREAD);
    mag -= SPREAD;
  }

  // remainder goes to actual slot
  IncrementSlot(i, mag);

  n_pts++;

  // update range
  x_min = std::min(x-1.5/m, x_min);
  x_max = std::max(x+1.5/m, x_max);
}

void Histogram::Reset(double minx, double maxx)
{
  assert(maxx > minx);
  b = minx;
  m = NUM_SLOTS/(maxx-minx);
  StoreReset();
  for (double x = minx; x<= maxx; x+= 1/m) {
    StoreAdd(x, 0);
  }
  n_pts = 0;
  x_min = 0;
  x_max = 0;
}

void Histogram::Clear()
{
  for (unsigned i=0; i< sum_n; ++i) {
    slots[i].y = 0;
  }
  n_pts = 0;
  x_min = 0;
  x_max = 0;
}

double Histogram::GetPercentile(const double p) const
{
  assert(p>= 0);
  assert(p<= 1);

  const double np = n_pts*p;
  double acc_n = 0;
  for (unsigned i=0; i+1< sum_n; ++i) {
    if (slots[i].y > np - acc_n) {
      const double residual = (np- acc_n)/slots[i].y;
      return slots[i+1].x * (residual) + slots[i].x* (1-residual)-0.5/m;
    }
    acc_n += slots[i].y;
  }

  // return mid point
  return b+ (sum_n-1)/(2*m);
}
