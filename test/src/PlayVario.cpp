// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/PCMPlayer.hpp"
#include "Audio/PCMPlayerFactory.hpp"
#include "Audio/VarioSynthesiser.hpp"
#include "ui/window/Init.hpp"
#include "system/Args.hpp"
#include "event/Loop.hxx"
#include "event/FineTimerEvent.hxx"
#include "DebugReplay.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <memory>

class ReplayTimer {
  FineTimerEvent timer;
  DebugReplay &replay;
  VarioSynthesiser &synthesiser;

public:
  ReplayTimer(EventLoop &event_loop,
              DebugReplay &_replay,
              VarioSynthesiser &_synthesiser)
    :timer(event_loop, BIND_THIS_METHOD(OnTimer)),
     replay(_replay), synthesiser(_synthesiser) {}

  ~ReplayTimer() {
    timer.Cancel();
  }

  auto &GetEventLoop() const noexcept {
    return timer.GetEventLoop();
  }

  void Start() {
    timer.Schedule(std::chrono::seconds(0));
  }

private:
  void OnTimer() noexcept {
    if (!replay.Next()) {
      GetEventLoop().Break();
      return;
    }

    auto vario = replay.Basic().brutto_vario;
    printf("%2.1f\n", (double)vario);
    synthesiser.SetVario(vario);

    timer.Schedule(std::chrono::seconds(1));
  }
};

int
main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  ScreenGlobalInit screen;

  EventLoop event_loop;

  std::unique_ptr<PCMPlayer> player(
      PCMPlayerFactory::CreateInstanceForDirectAccess(event_loop));

  const unsigned sample_rate = 44100;

  VarioSynthesiser synthesiser(sample_rate);

  if (!player->Start(synthesiser)) {
    fprintf(stderr, "Failed to start PCMPlayer\n");
    return EXIT_FAILURE;
  }

  ReplayTimer timer(event_loop, *replay, synthesiser);
  timer.Start();

  event_loop.Run();

  return EXIT_SUCCESS;
}
