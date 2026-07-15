// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"
#include "VarioAudioValue.hpp"

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
   * Update the values used by the audio vario.
   */
  void SetValue(const VarioAudioInput &input);

  /**
   * Is the audio vario platform available on this platform?
   * Must only be called after Initialise() has been called once before.
   */
  bool HaveAudioVario();

#else
  static inline void Initialise() {}
  static inline void Deinitialise() {}
  static inline void Configure([[maybe_unused]] const VarioSoundSettings &settings) {}
  static inline void SetValue([[maybe_unused]] const VarioAudioInput &input) {}
  static inline bool HaveAudioVario() { return false; }
#endif
};
