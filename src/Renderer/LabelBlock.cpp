// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LabelBlock.hpp"

inline bool
LabelBlock::Bucket::Check(const PixelRect rc) const noexcept
{
  for (auto i = blocks.begin(), end = blocks.end(); i != end; ++i)
    if (i->OverlapsWith(rc))
      return false;

  return true;
}

void
LabelBlock::reset() noexcept
{
  for (auto &i : buckets)
    i.Clear();
}

bool
LabelBlock::check(const PixelRect rc) noexcept
{
  /* Negative Y used to shift into a huge unsigned bucket and clamp
     to the last row, so labels along the top of the map collided
     with those along the bottom. */
  const unsigned y0 = rc.top < 0 ? 0 : unsigned(rc.top);
  const unsigned y1 = rc.bottom < 0 ? 0 : unsigned(rc.bottom);

  unsigned top = y0 >> BUCKET_SHIFT;
  unsigned bottom = y1 >> BUCKET_SHIFT;

  if (top >= BUCKET_COUNT)
    top = BUCKET_COUNT - 1;

  if (bottom >= BUCKET_COUNT)
    bottom = BUCKET_COUNT - 1;

  if (!buckets[bottom].Check(rc))
    return false;

  if (top != bottom && !buckets[top].Check(rc))
    return false;

  buckets[top].Add(rc);
  if (top != bottom)
    buckets[bottom].Add(rc);

  return true;
}
