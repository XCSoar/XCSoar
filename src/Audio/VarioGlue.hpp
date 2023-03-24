// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

struct VarioSoundSettings;

namespace AudioVarioGlue {
#ifdef HAVE_PCM_PLAYER

  /**
   * Global initialisation on startup.
   */
  void Initialise();

  /**
   * Global initialisation on shutdown.
   */
  void Deinitialise();

  /**
   * Update the configuration.  Must be called at least once, after
   * Initialise() and before SetValue().
   */
  void Configure(const VarioSoundSettings &settings);

  /**
   * Update the vario value.
   *
   * @param vario the current vario value [m/s]
   */
  void SetValue(double vario);

  /**
   * Declare that no vario value is known (e.g. when connection to all
   * devices is lost).  Vario sound will be shut off until vario
   * reception is back.
   */
  void NoValue();

  /**
   * Is the audio vario platform available on this platform?
   * Must only be called after Initialise() has been called once before.
   */
  bool HaveAudioVario();

#else
  static inline void Initialise() {}
  static inline void Deinitialise() {}
  static inline void Configure([[maybe_unused]] const VarioSoundSettings &settings) {}
  static inline void SetValue([[maybe_unused]] double vario) {}
  static inline void NoValue() {}
  static inline bool HaveAudioVario() { return false; }
#endif
};
