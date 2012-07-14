/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Replay/NmeaReplay.hpp"
#include "IO/LineReader.hpp"

#include <string.h>

NmeaReplay::NmeaReplay(NLineReader *_reader)
  :reader(_reader)
{
}

NmeaReplay::~NmeaReplay()
{
  delete reader;
}

void
NmeaReplay::Start()
{
  assert(!enabled);

  enabled = true;
}

bool
NmeaReplay::ReadUntilRMC(bool ignore)
{
  char *buffer;

  while ((buffer = reader->read()) != NULL) {
    if (!ignore)
      OnSentence(buffer);

    if (strstr(buffer, "$GPRMC") == buffer)
      return true;
  }

  return false;
}

bool
NmeaReplay::Update(fixed time_scale)
{
  assert(enabled);

  if (!UpdateTime())
    return true;

  for (fixed i = fixed_one; i <= time_scale; i += fixed_one) {
    enabled = ReadUntilRMC(i != time_scale);
    if (!enabled)
      return false;
  }

  assert(enabled);
  return true;
}

bool
NmeaReplay::UpdateTime()
{
  return true;
}
