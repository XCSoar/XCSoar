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
#include "ThermalSlice.hpp"
#include <math.h>
#include <assert.h>

void
ThermalSlice::Merge(const ThermalSlice& o)
{
     const double n_new = n + o.n;
     if (n_new>0) {
          w_n = (w_n*n + o.w_n*o.n)/n_new;
     } else {
          w_n = 0;
     }
     n = n_new;

     const double dt_new = dt + o.dt;
     if (dt_new>0) {
          w_t = (w_t*dt + o.w_t*o.dt)/dt_new;
     } else {
          w_t = 0;
     }
     dt = dt_new;
}

void
ThermalSlice::Update(const ThermalSlice& o, const double dh)
{
     dt = (o.time-time)*n;
     w_t = w_n = (dt != 0) ? dh*n/dt : o.w_n;
     dt = fabs(dt);
}

// whether this item has data
bool
ThermalSlice::Occupied() const
{
     return n>0;
}
