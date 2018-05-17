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

#include "FlarmNetDatabase.hpp"
#include "Util/StringUtil.hpp"

#include <assert.h>

void
FlarmNetDatabase::Insert(const FlarmNetRecord &record)
{
  FlarmId id = record.GetId();
  if (!id.IsDefined())
    /* ignore malformed records */
    return;

  map.insert(std::make_pair(id, record));
}

const FlarmNetRecord *
FlarmNetDatabase::FindFirstRecordByCallSign(const TCHAR *cn) const
{
  for (const auto &i : map) {
    assert(i.first.IsDefined());

    const FlarmNetRecord &record = i.second;
    if (StringIsEqual(record.callsign, cn))
      return &record;
  }

  return NULL;
}

unsigned
FlarmNetDatabase::FindRecordsByCallSign(const TCHAR *cn,
                                        const FlarmNetRecord *array[],
                                        unsigned size) const
{
  unsigned count = 0;

  for (const auto &i : map) {
    assert(i.first.IsDefined());

    const FlarmNetRecord &record = i.second;
    if (StringIsEqual(record.callsign, cn))
      array[count++] = &record;
  }

  return count;
}

unsigned
FlarmNetDatabase::FindIdsByCallSign(const TCHAR *cn, FlarmId array[],
                                    unsigned size) const
{
  unsigned count = 0;

  for (const auto &i : map) {
    assert(i.first.IsDefined());

    const FlarmNetRecord &record = i.second;
    if (StringIsEqual(record.callsign, cn))
      array[count++] = i.first;
  }

  return count;
}
