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

  max_thermal_height = fixed_zero;

  for (unsigned i = 0; i < NUMTHERMALBUCKETS; i++) {
    thermal_profile_w[i] = fixed_zero;
    thermal_profile_n[i] = 0;
  }
}

unsigned
ThermalBandInfo::BucketForHeight(fixed height) const
{
  if (negative(height))
    return 0;

  if (height >= max_thermal_height)
    return NUMTHERMALBUCKETS - 1;

  int bucket(NUMTHERMALBUCKETS * height / max_thermal_height);
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

  return bucket * max_thermal_height / NUMTHERMALBUCKETS;
}

void
ThermalBandInfo::Add(const fixed height, const fixed total_energy_vario)
{
  if (height > max_thermal_height) {
    // moved beyond ceiling, so redistribute buckets
    Expand(height);
  }

  const unsigned bucket = BucketForHeight(height);
  thermal_profile_w[bucket] += total_energy_vario;
  thermal_profile_n[bucket]++;
}

void
ThermalBandInfo::Expand(const fixed height)
{
  ThermalBandInfo new_tbi;

  // calculate new buckets so glider is below max
  fixed hbuk = max_thermal_height / NUMTHERMALBUCKETS;

  new_tbi.Clear();
  new_tbi.max_thermal_height = std::max(fixed_one, max_thermal_height);

  // increase ceiling until reach required height
  while (new_tbi.max_thermal_height < height) {
    new_tbi.max_thermal_height += hbuk;
  }

  // shift data into new buckets
  for (unsigned i = 0; i < NUMTHERMALBUCKETS; ++i) {
    const fixed h = BucketHeight(i);
    // height of center of bucket
    if (thermal_profile_n[i] > 0) {
      const unsigned j = new_tbi.BucketForHeight(h);
      new_tbi.thermal_profile_w[j] += thermal_profile_w[i];
      new_tbi.thermal_profile_n[j] += thermal_profile_n[i];
    }
  }

  *this = new_tbi;
}
