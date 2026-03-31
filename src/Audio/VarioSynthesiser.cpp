// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioSynthesiser.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

unsigned
VarioSynthesiser::VarioToFrequency(int ivario)
{
  if (ivario > 0) {
    const double x = (double)ivario / (double)max_vario;
    const double f = (double)zero_frequency +
      x * (double)(max_frequency - zero_frequency);
    return (unsigned)std::lround(f);
  } else if (ivario < 0) {
    const double x = (double)ivario / (double)min_vario; // both negative
    const double f = (double)zero_frequency -
      x * (double)(zero_frequency - min_frequency);
    return (unsigned)std::lround(f);
  } else
    return zero_frequency;
}

void
VarioSynthesiser::UpdateEnvelopeParameters()
{
  env_attack_samples = std::max(1u, sample_rate * ATTACK_MS / 1000);
  env_release_samples = std::max(1u, sample_rate * RELEASE_MS / 1000);
}

void
VarioSynthesiser::UpdateGateFromVario(int ivario)
{
  if (ivario <= 0) {
    /* sink/neutral: continuous */
    gate_on = true;
    gate_on_samples = 1;
    gate_off_samples = 0;
    gate_remaining = 0;
    return;
  }

  const unsigned period_samples = sample_rate *
    (min_period_ms + (max_vario - (unsigned)ivario) *
     (max_period_ms - min_period_ms) / (unsigned)max_vario) / 1000;

  /* 2/3 on, 1/3 off */
  const unsigned off = std::max(1u, period_samples / 3);
  const unsigned on = std::max(1u, period_samples - off);

  gate_on_samples = on;
  gate_off_samples = off;

  if (gate_remaining == 0) {
    gate_on = true;
    gate_remaining = gate_on_samples;
  }
}

void
VarioSynthesiser::SetVario(double vario)
{
  const std::lock_guard lock{mutex};

  UpdateEnvelopeParameters();

  const int ivario = std::clamp((int)lround(vario * 100), min_vario, max_vario);

  if (dead_band_enabled && InDeadBand(ivario)) {
    env_target = 0;
    return;
  }

  SetTone(VarioToFrequency(ivario));
  UpdateGateFromVario(ivario);

  /* set target based on gate */
  env_target = gate_on ? ENV_MAX : 0;
}

void
VarioSynthesiser::SetSilence()
{
  const std::lock_guard lock{mutex};
  env_target = 0;
}


void
VarioSynthesiser::Synthesise(int16_t *buffer, size_t n)
{
  const std::lock_guard lock{mutex};

  assert(env_attack_samples > 0);
  assert(env_release_samples > 0);

  ToneSynthesiser::Synthesise(buffer, n);

  for (size_t i = 0; i < n; ++i) {
    /* gate timing (climb beep mode) */
    if (gate_off_samples > 0) {
      if (gate_remaining == 0) {
        gate_on = !gate_on;
        gate_remaining = gate_on ? gate_on_samples : gate_off_samples;
        env_target = gate_on ? ENV_MAX : 0;
      }

      --gate_remaining;
    }

    /* envelope */
    if (env < env_target) {
      const int step = std::max(1, ENV_MAX / (int)env_attack_samples);
      env = std::min(env_target, env + step);
    } else if (env > env_target) {
      const int step = std::max(1, ENV_MAX / (int)env_release_samples);
      env = std::max(env_target, env - step);
    }

    buffer[i] = (int16_t)((int)buffer[i] * env / ENV_MAX);
  }

  if (env == 0)
    Restart();
}
