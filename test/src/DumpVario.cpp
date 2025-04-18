// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/VarioSynthesiser.hpp"
#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "util/Macros.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  const unsigned sample_rate = 44100;

  VarioSynthesiser synthesiser(sample_rate);

  while (replay->Next()) {
    auto vario = replay->Basic().brutto_vario;
    synthesiser.SetVario(vario);

    static int16_t buffer[sample_rate];
    synthesiser.Synthesise(buffer, ARRAY_SIZE(buffer));

    if (write(1, buffer, sizeof(buffer)) < 0) {
      perror("write");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
