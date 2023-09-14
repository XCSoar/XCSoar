// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
