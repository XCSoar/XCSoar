// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/VarioSynthesiser.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <array>

static bool
IsSilent(double vario, bool dead_band_enabled,
         double min_dead=-0.3, double max_dead=0.1)
{
  VarioSynthesiser synthesiser(44100);
  synthesiser.SetDeadBand(dead_band_enabled);
  synthesiser.SetDeadBandRange(min_dead, max_dead);
  synthesiser.SetVario(vario);

  std::array<int16_t, 512> buffer{};
  synthesiser.Synthesise(buffer.data(), buffer.size());
  return std::all_of(buffer.begin(), buffer.end(),
                     [](int16_t sample) { return sample == 0; });
}

int
main()
{
  plan_tests(5);

  ok1(!IsSilent(0, false));
  ok1(IsSilent(0, true));
  ok1(IsSilent(-0.3, true));
  ok1(IsSilent(0.1, true));
  ok1(!IsSilent(0.2, true));

  return exit_status();
}
