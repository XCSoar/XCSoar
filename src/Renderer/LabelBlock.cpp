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
  unsigned top = rc.top >> BUCKET_SHIFT;
  unsigned bottom = rc.bottom >> BUCKET_SHIFT;

  if (top >= BUCKET_COUNT)
    top = BUCKET_COUNT - 1;

  if (bottom < BUCKET_COUNT && !buckets[bottom].Check(rc))
    return false;

  if (top < bottom && !buckets[top].Check(rc))
    return false;

  buckets[top].Add(rc);
  if (bottom < BUCKET_COUNT && top != bottom)
    buckets[bottom].Add(rc);

  return true;
}
