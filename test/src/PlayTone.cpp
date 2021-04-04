/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Audio/Features.hpp"

#ifndef HAVE_PCM_PLAYER
#error PCMPlayer not available
#endif

#include "Audio/PCMPlayer.hpp"
#include "Audio/PCMPlayerFactory.hpp"
#include "Audio/ToneSynthesiser.hpp"
#include "ui/window/Init.hpp"
#include "event/Loop.hxx"
#include "event/FineTimerEvent.hxx"
#include "system/Args.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <memory>

struct Instance {
  EventLoop event_loop;

  std::unique_ptr<PCMPlayer> player{PCMPlayerFactory::CreateInstanceForDirectAccess(event_loop)};

  FineTimerEvent stop_timer{event_loop, BIND_THIS_METHOD(OnStopTimer)};

  Instance() noexcept {
    stop_timer.Schedule(std::chrono::seconds(1));
  }

  void OnStopTimer() noexcept {
    player->Stop();
    event_loop.Break();
  }
};

int
main(int argc, char **argv)
{
  Args args(argc, argv, "HZ");
  const char *freq_s = args.ExpectNext();
  args.ExpectEnd();

  unsigned freq = strtoul(freq_s, NULL, 10);
  if (freq == 0 || freq > 48000) {
    fprintf(stderr, "Invalid frequency\n");
    return EXIT_FAILURE;
  }

  ScreenGlobalInit screen;

  Instance instance;

  const unsigned sample_rate = 44100;

  ToneSynthesiser tone(sample_rate);
  tone.SetTone(freq);

  if (!instance.player->Start(tone)) {
    fprintf(stderr, "Failed to start PCMPlayer\n");
    return EXIT_FAILURE;
  }

  instance.event_loop.Run();

  return EXIT_SUCCESS;
}
