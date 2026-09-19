// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/SDLPCMPlayer.hpp"
#include "Audio/PCMDataSource.hpp"
#include "TestUtil.hpp"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <atomic>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

class Source final : public PCMDataSource {
  const unsigned sample_rate;
  size_t remaining;

public:
  std::atomic<unsigned> calls{0};
  std::atomic<bool> exhausted{false};

  explicit Source(unsigned _sample_rate, size_t frames = 100)
    :sample_rate(_sample_rate), remaining(frames) {}

  bool IsBigEndian() const override {
    return false;
  }

  unsigned GetSampleRate() const override {
    return sample_rate;
  }

  size_t GetData(int16_t *buffer, size_t n) override {
    ++calls;
    const size_t read = std::min(n, remaining);
    std::fill_n(buffer, read, 1234);
    remaining -= read;
    if (read < n)
      exhausted = true;
    return read;
  }

  bool WaitUntilExhausted() const {
    const auto deadline = SDL_GetTicks() + 2000;
    while (!exhausted && SDL_GetTicks() < deadline)
      SDL_Delay(1);
    return exhausted;
  }
};

int
main()
{
  plan_tests(13);

#if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
  skip(13, 0,
       "SDL dummy audio driver has no callback timing on iOS Simulator");
  return exit_status();
#endif

  SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
  if (!SDL_Init(SDL_INIT_AUDIO)) {
    skip(13, 0, "SDL dummy audio driver unavailable");
    return exit_status();
  }

  {
    /* Sources outlive the player, including any outstanding callback. */
    Source first(44100), replacement(44100), changed_rate(22050);
    Source restarted(44100), empty(44100, 0);
    SDLPCMPlayer player;

    ok1(player.Start(first));
    ok1(first.WaitUntilExhausted());
    const auto first_calls = first.calls.load();
    SDL_Delay(30);
    ok1(first.calls == first_calls);

    ok1(player.Start(replacement));
    ok1(replacement.WaitUntilExhausted());
    ok1(first.calls == first_calls);

    ok1(player.Start(changed_rate));
    ok1(changed_rate.WaitUntilExhausted());
    player.Stop();
    const auto stopped_calls = changed_rate.calls.load();
    SDL_Delay(30);
    ok1(changed_rate.calls == stopped_calls);

    player.Stop();
    ok1(player.Start(restarted));
    ok1(restarted.WaitUntilExhausted());
    ok1(player.Start(empty));
    ok1(empty.WaitUntilExhausted());
  }

  SDL_Quit();
  return exit_status();
}
