/* Copyright_License {

   XCSoar Glide Computer - http://www.xcsoar.org/
   Copyright (C) 2000-2021 The XCSoar Project
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

#include "ThermalBand.hpp"

#include <algorithm>
#include <cassert>

void
ThermalBand::Reset()
{
  dh = 10;
  h_min = 0;
  slices.clear();
}

double
ThermalBand::GetSliceCenter(const unsigned index) const
{
  assert(index < size());
  if (index+1 == size()) {
    return GetSliceHeight(index-1) + slices[index].n*dh + dh/2;
  }
  return GetSliceHeight(index) + dh/2;
}

void
ThermalBand::Update(const unsigned index)
{
  assert(index < size());
  if (index + 1 < size()) {
    slices[index].Update(slices[index + 1], dh);
  } else if (index) {
    slices[index].Update(slices[index - 1], -dh);
  }
}

void
ThermalBand::CheckExpand(const ThermalBand& tb, bool update)
{
  assert(!empty());

  // check to see if ceiling will fit into range
  while (GetSliceIndex(tb.GetCeiling()) + 1 >= slices.capacity()) {
    Decimate(update);
  }
}

void
ThermalBand::Decimate(bool update)
{
  assert(!empty());

  dh *= 2;
  const unsigned new_size = (size() + 1) / 2;
  for (unsigned i = 0; i < new_size; ++i) {
    ThermalSlice s = slices[i * 2];
    if (i * 2 + 1 < size()) {
      s.Merge(slices[i * 2 + 1]);
    }
    slices[i] = s;
  }
  slices.resize(new_size);

  for (unsigned i=0; i< size(); ++i) {
    slices[i].n /= 2;
    if (update)
      Update(i);
  }
}

////////////////// supplemental information ///////////////////////

double
ThermalBand::GetMaxN() const
{
  double n = 0;
  for (const auto &i : slices)
    n = std::max(n, i.n);
  return n;
}

double
ThermalBand::GetTimeElapsed() const
{
  // note: don't include top slice as this is estimated
  double t = 0;
  for (unsigned i = 0; i + 1 < size(); ++i) {
    t += slices[i].dt;
  }
  return t;
}

double
ThermalBand::GetMaxW() const
{
  double w = 0;
  for (unsigned i = 0; i + 1 < size(); ++i) {
    w = std::max(w, slices[i].w_n);
  }
  return w;
}
