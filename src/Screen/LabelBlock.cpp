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

// simple code to prevent text writing over map city names
#include "Screen/LabelBlock.hpp"

void
LabelBlock::Bucket::Clear()
{
  blocks.clear();
}

static gcc_pure bool
CheckRectOverlap(const PixelRect& rc1, const PixelRect& rc2)
{
  return rc1.left < rc2.right && rc1.right > rc2.left &&
    rc1.top < rc2.bottom && rc1.bottom > rc2.top;
}

bool
LabelBlock::Bucket::Check(const PixelRect rc) const
{
  for (BlockArray::const_iterator i = blocks.begin(), end = blocks.end();
       i != end; ++i)
    if (CheckRectOverlap(*i, rc))
      return false;

  return true;
}

void LabelBlock::reset()
{
  for (unsigned i = 0; i < BUCKET_COUNT; ++i)
    buckets[i].Clear();
}

bool LabelBlock::check(const PixelRect rc)
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
  return true;
}
