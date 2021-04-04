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
