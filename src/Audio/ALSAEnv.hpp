// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace ALSAEnv
{
  /**
   * Get the ALSA device name. This is normally "default" (which is usually
   * the dmix plugin, or PulseAudio).
   * Some users might want to explicitly specify a hw (or plughw) device
   * for reduced latency. This can be done using the environment variable
   * "ALSA_DEVICE".
   *
   * @return Value of the environment variable "ALSA_DEVICE", or "default" if
   * not set.
   */
  const char *GetALSADeviceName();

  /**
   * Get the desired ALSA latency in μs. The ALSA PCM buffer size and period
   * time is calculated using this value.
   *
   * We always want low latency. But lower latency values result in a smaller
   * buffer size, more frequent interrupts, and a higher risk for buffer
   * underruns.
   *
   * @return Value of the environment variable "ALSA_LATENCY", parsed as
   * unsigned, or 10000 if not set, or unparsable. The unit is μs.
   */
  unsigned GetALSALatency();
}
