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
#include "ThermalEncounterCollection.hpp"
#include <algorithm>
#include <assert.h>

void
ThermalEncounterCollection::Merge(const ThermalBand& tb)
{
     if (!tb.Valid()) {
          // nothing to do
          return;
     }

     if (!size()) {
          // accept whole item as direct copy
          Copy(tb);
          UpdateTimes();
          return;
     }

     // check for floor lowering with new data
     LowerFloor(tb.GetFloor());

     // ensure there's enough room for new data
     CheckExpand(tb, false);

     // insert all items
     MergeUnsafe(tb);

     UpdateTimes();
}

void
ThermalEncounterCollection::LowerFloor(const double new_floor)
{
     // floor is already low enough, no action required
     if (h_min <= new_floor) {
          return;
     }

     const double delta_move = h_min - new_floor;

     // ensure there's enough space for new elements
     while (size()+ (int)delta_move/dh > slices.capacity()) {
          Decimate(false);
     }

     // move elements up tree
     const unsigned index_move = delta_move/dh;
     if (index_move) {
          const unsigned prev_size = size();
          slices.resize(prev_size + index_move);
          for (int i = prev_size-1; i>= 0; --i) {
               slices[i+index_move]= slices[i];
          }
          for (unsigned i = 0; i< index_move; ++i) {
               slices[i].Reset();
          }
     }
     // adjust base according to how much moved
     h_min -= index_move*dh;
}

void
ThermalEncounterCollection::MergeUnsafe(const ThermalBand& o)
{
     // Progressively merge in new data, iterating over new data band.
     //
     // This applies correction factor to number of encounters in order to
     // accommodate differing spacing between this and the other band.
     //
     // It also accounts for slices in other being finer than this, merging them
     // together before merging with this.
     //
     // And, it also accounts for slices in other being coarser than this,
     // applying them to all relevant slices in this.

     const double range_factor = o.GetSpacing()/GetSpacing();
     const unsigned num_o = o.size();

     ThermalSlice acc;
     int n = 0;
     acc.Reset();

     for (unsigned i=0; i< num_o; ++i) {

          // index of this slice at height of other
          const unsigned index = GetSliceIndex(o.GetSliceHeight(i));

          // index of this slice at next height of other
          const unsigned index_end = (i+1 < num_o)? GetSliceIndex(o.GetSliceHeight(i+1)) : index+1;

          // merge in other's slice to local accumulator
          acc.Merge(o.GetSlice(i));
          n++;

          // update this only if we're finished with other's items within this slice
          if (index_end > index) {

               // apply correction factor
               acc.n *= range_factor/n;

               // merge or append to this as required to all relevant slices in this
               for (unsigned j=index; j< index_end; ++j) {
                    if (j >= size()) {
                         slices.append(acc);
                    } else {
                         slices[j].Merge(acc);
                    }
               }

               // reset accumulator for next index of this
               acc.Reset();
               n = 0;
          }
     }
}


void
ThermalEncounterCollection::UpdateTimes()
{
     assert(size());
     // update accumulated times below slice height
     double t = slices[0].time = 0;
     for (unsigned i=1; i< size(); ++i) {
          t += slices[i-1].dt;
          slices[i].time = t;
     }
}
