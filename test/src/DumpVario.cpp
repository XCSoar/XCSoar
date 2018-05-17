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

#include "Audio/VarioSynthesiser.hpp"
#include "OS/Args.hpp"
#include "DebugReplay.hpp"
#include "Util/Macros.hpp"

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
