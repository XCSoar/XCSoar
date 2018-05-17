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

#include "ToneSynthesiser.hpp"
#include "Math/FastTrig.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

void
ToneSynthesiser::SetTone(unsigned tone_hz)
{
  increment = ARRAY_SIZE(ISINETABLE) * tone_hz / sample_rate;
}

void
ToneSynthesiser::Synthesise(int16_t *buffer, size_t n)
{
  assert(angle < ARRAY_SIZE(ISINETABLE));

  for (int16_t *end = buffer + n; buffer != end; ++buffer) {
    *buffer = ISINETABLE[angle] * (32767 / 1024) * (int)volume / 100;
    angle = (angle + increment) & (ARRAY_SIZE(ISINETABLE) - 1);
  }
}

unsigned
ToneSynthesiser::ToZero() const
{
  assert(angle < ARRAY_SIZE(ISINETABLE));

  if (angle < increment)
    /* close enough */
    return 0;

  return (ARRAY_SIZE(ISINETABLE) - angle) / increment;
}
