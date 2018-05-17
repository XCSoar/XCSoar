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

#ifndef XCSOAR_AUDIO_VOLUME_CONTROLLER_HPP
#define XCSOAR_AUDIO_VOLUME_CONTROLLER_HPP


#include "Features.hpp"

#include "Thread/Mutex.hpp"

#if defined(HAVE_EXT_VOLUME_CONTROLLER) && defined(ENABLE_ALSA)
#include <alsa/asoundlib.h>
#endif


/**
 * Helper class for controlling master audio volume.
 *
 * It can control two types of audio volume levels:
 * * Internal (software-side) volume level (via #PCMMixer).
 * * External volume: If HAVE_EXT_VOLUME_CONTROLLER is defined, this component
 *   takes control of the audio device's volume level. Controlling this volume
 *   is usually not desired on a general-purpose OS, where the user can use a
 *   volume controller application, whose setting should be respected and should
 *   not be overridden. But can be useful for embedded systems, where no such
 *   system-wide volume controller exists.
 *
 * Volume levels can be higher than 100% (see GetMaxValue()). The meaning of
 * this is: volume levels up to 100% are considered "safe", which means that
 * normally no cipping artifacts can occur. Using higher volume levels does
 * likely cause clipping artifacts.
 *
 * All public functions are thread-safe.
 */
class VolumeController final {
#ifdef HAVE_EXT_VOLUME_CONTROLLER
#ifdef ENABLE_ALSA
  Mutex alsa_lock;
  bool alsa_mixer_initialised = false;
  snd_mixer_t *alsa_mixer_handle = nullptr;
  snd_mixer_elem_t *ext_master_volume_ctl = nullptr;
  long ext_master_min, ext_master_max, ext_master_zero_db;
  bool ext_master_ctl_has_vol, ext_master_ctl_has_switch;

  bool SetExternalVolumeNoLock(unsigned vol_percent);

  bool InitExternalVolumeControl(unsigned initial_vol_percent);

  /**
   * Set the sound device's volume.
   *
   * @return true, if volume control was successful, false if an error occured,
   * of if no master volume control is available. It also returns false, if
   * there is only a master switch, even if switching on / off was successful.
   */
  bool SetExternalVolume(unsigned vol_percent);
#else
#error No external volume controller implementation available
#endif
  void SetInternalVolume(unsigned vol_percent, bool have_external_control);
#endif
  void SetInternalVolume(unsigned vol_percent);

public:
  VolumeController() = default;

#if defined(HAVE_EXT_VOLUME_CONTROLLER) && defined(ENABLE_ALSA)
  ~VolumeController();
#endif

  VolumeController(const VolumeController&) = delete;
  VolumeController& operator=(const VolumeController&) = delete;

  /**
   * Maximum volume level which can be used as the SetVolume() parameter.
   */
  static constexpr unsigned GetMaxValue() {
    return 150;
  }

  /**
   * Set volume level (internal and, if available, external).
   *
   * @param vol_percent volume value in percent. Must be between 0 and
   * GetMaxValue().
   */
  void SetVolume(unsigned vol_percent);
};

#endif
