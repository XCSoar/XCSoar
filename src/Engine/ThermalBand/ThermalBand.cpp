// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalBand.hpp"

#include <algorithm>
#include <cassert>

void
ThermalBand::Reset() noexcept
{
  dh = 10;
  h_min = 0;
  slices.clear();
}

double
ThermalBand::GetSliceCenter(const unsigned index) const noexcept
{
  assert(index < size());
  if (index+1 == size()) {
    return GetSliceHeight(index-1) + slices[index].n*dh + dh/2;
  }
  return GetSliceHeight(index) + dh/2;
}

void
ThermalBand::Update(const unsigned index) noexcept
{
  assert(index < size());
  if (index + 1 < size()) {
    slices[index].Update(slices[index + 1], dh);
  } else if (index) {
    slices[index].Update(slices[index - 1], -dh);
  }
}

void
ThermalBand::CheckExpand(const ThermalBand &tb, bool update) noexcept
{
  assert(!empty());

  // check to see if ceiling will fit into range
  while (GetSliceIndex(tb.GetCeiling()) + 1 >= slices.capacity()) {
    Decimate(update);
  }
}

void
ThermalBand::Decimate(bool update) noexcept
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
ThermalBand::GetMaxN() const noexcept
{
  double n = 0;
  for (const auto &i : slices)
    n = std::max(n, i.n);
  return n;
}

FloatDuration
ThermalBand::GetTimeElapsed() const noexcept
{
  // note: don't include top slice as this is estimated
  FloatDuration t{};
  for (unsigned i = 0; i + 1 < size(); ++i) {
    t += slices[i].dt;
  }
  return t;
}

double
ThermalBand::GetMaxW() const noexcept
{
  double w = 0;
  for (unsigned i = 0; i + 1 < size(); ++i) {
    w = std::max(w, slices[i].w_n);
  }
  return w;
}
