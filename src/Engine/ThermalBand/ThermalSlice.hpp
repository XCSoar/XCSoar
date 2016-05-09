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
#ifndef THERMAL_SLICE_HPP
#define THERMAL_SLICE_HPP

#include <type_traits>

/*
  Structure to maintain information about average climb rates through a particular
  altitude.  Handles both time-averaged and encounter-averaged climb rates.

 */
struct ThermalSlice {

     // Set the time of passage through this slice.
     void SetSample(double _time) {
          time = _time;
          n = 1.;
     }

     // climb rate averaged by time [m/s]
     double w_t;

     // climb rate averaged by number of encounters [m/s]
     double w_n;

     // number of encounters (passages through this slice).  This is real valued as
     // fractional values are calculated during merging, and also to track the proportion
     // of travel to the next (as yet unvisited) slice.
     double n;

     // Time accumulated climbing up to this slice. [s]
     double time;

     // Time to arrive at next slice [s]
     double dt;

     // combine this slice with the other
     void Merge(const ThermalSlice& o);

     // update climb statistics other slice's time and height difference
     void Update(const ThermalSlice& o, const double dh);

     void Reset() {
          w_n=0;
          w_t=0;
          n=0;
          time=0;
          dt=0;
     }

     // whether this item has data
     bool Occupied() const;
};

static_assert(std::is_trivial<ThermalSlice>::value, "type is not trivial");

#endif
