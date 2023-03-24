// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ToneSynthesiser.hpp"
#include "Math/FastTrig.hpp"

#include <cassert>

void
ToneSynthesiser::SetTone(unsigned tone_hz)
{
  increment = ISINETABLE.size() * tone_hz / sample_rate;
}

void
ToneSynthesiser::Synthesise(int16_t *buffer, size_t n)
{
  assert(angle < ISINETABLE.size());

  for (int16_t *end = buffer + n; buffer != end; ++buffer) {
    *buffer = ISINETABLE[angle] * (32767 / 1024) * (int)volume / 100;
    angle = (angle + increment) & (ISINETABLE.size() - 1);
  }
}

unsigned
ToneSynthesiser::ToZero() const
{
  assert(angle < ISINETABLE.size());

  if (angle < increment)
    /* close enough */
    return 0;

  return (ISINETABLE.size() - angle) / increment;
}
