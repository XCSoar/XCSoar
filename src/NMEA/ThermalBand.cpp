/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include <assert.h>

void
ThermalBandInfo::Clear()
{
  working_band_height = working_band_ceiling = fixed_zero;
  working_band_fraction = fixed_zero;

  MaxThermalHeight = fixed_zero;

  for (unsigned i = 0; i < NUMTHERMALBUCKETS; i++) {
    ThermalProfileW[i] = fixed_zero;
    ThermalProfileN[i] = 0;
  }
}

unsigned
ThermalBandInfo::BucketForHeight(fixed height) const
{
  if (negative(height))
    return 0;

  if (height >= MaxThermalHeight)
    return NUMTHERMALBUCKETS - 1;

  int bucket = NUMTHERMALBUCKETS * height / MaxThermalHeight;
  if (bucket < 0)
    return 0;

  if ((unsigned)bucket >= NUMTHERMALBUCKETS)
    return NUMTHERMALBUCKETS - 1;

  return bucket;
}

fixed
ThermalBandInfo::BucketHeight(unsigned bucket) const
{
  assert(bucket < NUMTHERMALBUCKETS);

  return bucket * MaxThermalHeight / NUMTHERMALBUCKETS;
}

void
ThermalBandInfo::Add(const fixed height, const fixed total_energy_vario)
{
  if (height > MaxThermalHeight) {
    // moved beyond ceiling, so redistribute buckets
    Expand(height);
  }

  const unsigned bucket = BucketForHeight(height);
  ThermalProfileW[bucket] += total_energy_vario;
  ThermalProfileN[bucket]++;
}

void
ThermalBandInfo::Expand(const fixed height)
{
  ThermalBandInfo new_tbi;

  // calculate new buckets so glider is below max
  fixed hbuk = MaxThermalHeight / NUMTHERMALBUCKETS;

  new_tbi.Clear();
  new_tbi.MaxThermalHeight = std::max(fixed_one, MaxThermalHeight);

  // increase ceiling until reach required height
  while (new_tbi.MaxThermalHeight < height) {
    new_tbi.MaxThermalHeight += hbuk;
  }

  // shift data into new buckets
  for (unsigned i = 0; i < NUMTHERMALBUCKETS; ++i) {
    const fixed h = BucketHeight(i);
    // height of center of bucket
    if (ThermalProfileN[i] > 0) {
      const unsigned j = new_tbi.BucketForHeight(h);
      new_tbi.ThermalProfileW[j] += ThermalProfileW[i];
      new_tbi.ThermalProfileN[j] += ThermalProfileN[i];
    }
  }

  *this = new_tbi;
}
