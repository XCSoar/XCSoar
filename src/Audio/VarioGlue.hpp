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

#ifndef XCSOAR_AUDIO_VARIO_GLUE_HPP
#define XCSOAR_AUDIO_VARIO_GLUE_HPP

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
  static inline void Configure(const VarioSoundSettings &settings) {}
  static inline void SetValue(double vario) {}
  static inline void NoValue() {}
  static inline bool HaveAudioVario() { return false; }
#endif
};

#endif
