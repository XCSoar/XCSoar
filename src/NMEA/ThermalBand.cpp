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

#include "ThermalBand.hpp"

#include <algorithm>

#include <assert.h>

void
ThermalBandInfo::Clear()
{
  working_band_height = working_band_ceiling = 0;
  working_band_fraction = 0;

  max_thermal_height = 0;

  thermal_profile_n.fill(0);
  thermal_profile_w.fill(0);
}

unsigned
ThermalBandInfo::BucketForHeight(double height) const
{
  if (height < 0)
    return 0;

  if (height >= max_thermal_height)
    return N_BUCKETS - 1;

  int bucket(N_BUCKETS * height / max_thermal_height);
  if (bucket < 0)
    return 0;

  if ((unsigned)bucket >= N_BUCKETS)
    return N_BUCKETS - 1;

  return bucket;
}

double
ThermalBandInfo::BucketHeight(unsigned bucket) const
{
  assert(bucket < N_BUCKETS);

  return bucket * max_thermal_height / N_BUCKETS;
}

void
ThermalBandInfo::Add(const double height, const double total_energy_vario)
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
ThermalBandInfo::Expand(const double height)
{
  ThermalBandInfo new_tbi;

  // calculate new buckets so glider is below max
  auto hbuk = max_thermal_height / N_BUCKETS;

  new_tbi.Clear();
  new_tbi.max_thermal_height = std::max(1., max_thermal_height);

  // increase ceiling until reach required height
  while (new_tbi.max_thermal_height < height) {
    new_tbi.max_thermal_height += hbuk;
  }

  // shift data into new buckets
  for (unsigned i = 0; i < N_BUCKETS; ++i) {
    const auto h = BucketHeight(i);
    // height of center of bucket
    if (thermal_profile_n[i] > 0) {
      const unsigned j = new_tbi.BucketForHeight(h);
      new_tbi.thermal_profile_w[j] += thermal_profile_w[i];
      new_tbi.thermal_profile_n[j] += thermal_profile_n[i];
    }
  }

  *this = new_tbi;
}
