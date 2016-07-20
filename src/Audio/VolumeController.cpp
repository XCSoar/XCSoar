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

#include "VolumeController.hpp"

#include "ALSAEnv.hpp"
#include "GlobalPCMMixer.hpp"
#include "PCMMixer.hpp"

#include "LogFile.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"

#include <assert.h>

#include <list>

#include <boost/assert.hpp>


#if defined(HAVE_EXT_VOLUME_CONTROLLER) && defined(ENABLE_ALSA)

static constexpr char PCM_CONTROL_NAME[] = "pcm";

/* List of ALSA mixer element names (case-insensitive), which are typically good
 * candidates for being the master volume controller. Unfortunately, these names
 * are not standardized, so there might exist aound devices whose master volume
 * controller has a different name, but this list should be fine to hande most
 * audio devices.
 *
 * Inspired by https://www.kernel.org/doc/Documentation/sound/alsa/ControlNames.txt */
static constexpr const char *CONTROL_NAMES_PRIORITY[] =
    { "master",
      "master mono",
      "hardware master",
      "master digital",
      "speaker",
      "headphone",
      "hdmi/dp",
      "hdmi",
      "dp",
      "spdif",
      "mono",
      "mono output",
      "iec958",
      "line out",
      "playback",
      "dac",
      "wave",
      PCM_CONTROL_NAME };


VolumeController::~VolumeController()
{
  if (nullptr != alsa_mixer_handle)
    BOOST_VERIFY(0 == snd_mixer_close(alsa_mixer_handle));
}

bool
VolumeController::SetExternalVolumeNoLock(unsigned vol_percent)
{
  assert(alsa_lock.IsLockedByCurrent());
  assert(nullptr != alsa_mixer_handle);

  if (nullptr == ext_master_volume_ctl)
    return false;
  else {
    long ext_vol;

    if (0 == vol_percent)
      ext_vol = ext_master_min;
    else if (100 == vol_percent)
      ext_vol = ext_master_zero_db;
    else if (GetMaxValue() == vol_percent)
      ext_vol = ext_master_max;
    else if (vol_percent < 100)
      ext_vol = ext_master_min + (ext_master_zero_db - ext_master_min) *
          static_cast<float>(vol_percent) / 100;
    else
      ext_vol = ext_master_min + (ext_master_max - ext_master_min) *
          static_cast<float>(vol_percent) / GetMaxValue();

    if (ext_master_ctl_has_vol)
      BOOST_VERIFY(0 == snd_mixer_selem_set_playback_volume_all(
                            ext_master_volume_ctl, ext_vol));

    if (ext_master_ctl_has_switch)
      BOOST_VERIFY(0 == snd_mixer_selem_set_playback_switch_all(
                            ext_master_volume_ctl,
                            (0 == vol_percent) ? 0 : 1));

    return ext_master_ctl_has_vol;
  }
}

bool
VolumeController::SetExternalVolume(unsigned vol_percent)
{
  const ScopeLock protect(alsa_lock);
  if (alsa_mixer_initialised)
    return SetExternalVolumeNoLock(vol_percent);
  else
    return InitExternalVolumeControl(vol_percent);
}

bool
VolumeController::InitExternalVolumeControl(unsigned initial_vol_percent)
{
  assert(alsa_lock.IsLockedByCurrent());
  assert(!alsa_mixer_initialised);
  assert(nullptr == alsa_mixer_handle);
  assert(nullptr == ext_master_volume_ctl);

  // Do not retry initialisation later, even if it fails.
  alsa_mixer_initialised = true;

  int alsa_error = snd_mixer_open(&alsa_mixer_handle, 0);
  if (0 != alsa_error) {
    LogFormat("snd_mixer_open(0x%p, 0) failed: %d - %s",
              &alsa_mixer_handle,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }
  assert(nullptr != alsa_mixer_handle);

  const char *alsa_device_name = ALSAEnv::GetALSADeviceName();
  alsa_error = snd_mixer_attach(alsa_mixer_handle, alsa_device_name);
  if (0 != alsa_error) {
    LogFormat("snd_mixer_attach(0x%p, \"%s\") failed: %d - %s",
              alsa_mixer_handle,
              alsa_device_name,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  alsa_error = snd_mixer_selem_register(alsa_mixer_handle, nullptr, nullptr);
  if (0 != alsa_error) {
    LogFormat("snd_mixer_selem_register(0x%p, nullptr, nullptr) failed: "
                  "%d - %s",
              alsa_mixer_handle,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  alsa_error = snd_mixer_load(alsa_mixer_handle);
  if (0 != alsa_error) {
    LogFormat("snd_mixer_load(0x%p) failed: %d - %s",
              alsa_mixer_handle,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  /* Some sound devices have a "PCM" volume control, which is not necessarily
   * to control the master volume. But it if present, we must ensure that it
   * is not muted. */
  snd_mixer_elem_t *pcm_volume_ctl = nullptr;

  /* Collect ALSA mixer elements which could be the right one to control
   * master volume. */
  std::list<snd_mixer_elem_t *> elements;
  for (snd_mixer_elem_t *elem = snd_mixer_first_elem(alsa_mixer_handle);
       nullptr != elem;
       elem = snd_mixer_elem_next(elem)) {
    const char *elem_name = snd_mixer_selem_get_name(elem);
    if (snd_mixer_selem_is_active(elem) &&
        (nullptr != elem_name) &&
        (snd_mixer_selem_has_playback_volume(elem) ||
            snd_mixer_selem_has_playback_switch(elem))) {
      if ((nullptr == pcm_volume_ctl) &&
          (StringIsEqualIgnoreCase(PCM_CONTROL_NAME, elem_name)))
        pcm_volume_ctl = elem;
      elements.push_back(elem);
    }
  }

  if (0 == elements.size()) {
    LogFormat("No usable master volume control found for ALSA device \"%s\"",
              alsa_device_name);
    return false;
  }

  if (elements.size() > 1) {
    /* Finding the right mixer element to control master volume is unfortunately
     * quite tricky. This sort algorithm does hopefully bring the to the right
     * mixer element to the back of our list */
    elements.sort([](snd_mixer_elem_t *a, snd_mixer_elem_t *b) {
      /* Most important criterion: It should provide playback volume control,
       * not only a switch. */
      if (!snd_mixer_selem_has_playback_volume(a) &&
          snd_mixer_selem_has_playback_volume(b))
        return true;

      const char *a_name = snd_mixer_selem_get_name(a);
      const char *b_name = snd_mixer_selem_get_name(b);
      assert(nullptr != a_name);
      assert(nullptr != b_name);

      /* Use our CONTROL_NAMES_PRIORITY list and prefer elements which are in
       * front of this list. */
      int a_name_prio = -1, b_name_prio = -1;
      for (unsigned i = 0;
           (-1 == a_name_prio) &&
               (-1 == b_name_prio) &&
               (i < ARRAY_SIZE(CONTROL_NAMES_PRIORITY));
           ++i) {
        if (StringIsEqualIgnoreCase(CONTROL_NAMES_PRIORITY[i], a_name))
          a_name_prio = static_cast<int>(i);
        if (StringIsEqualIgnoreCase(CONTROL_NAMES_PRIORITY[i], b_name))
          b_name_prio = static_cast<int>(i);
      }
      return a_name_prio < b_name_prio;
    });
  }

  ext_master_volume_ctl = elements.back();
  ext_master_ctl_has_vol =
      (0 != snd_mixer_selem_has_playback_volume(ext_master_volume_ctl));
  ext_master_ctl_has_switch =
      (0 != snd_mixer_selem_has_playback_switch(ext_master_volume_ctl));

  if (ext_master_ctl_has_vol) {
    BOOST_VERIFY(0 == snd_mixer_selem_get_playback_volume_range(
                          ext_master_volume_ctl,
                          &ext_master_min,
                          &ext_master_max));
    if (0 != snd_mixer_selem_ask_playback_dB_vol(
                 ext_master_volume_ctl, 0, 0, &ext_master_zero_db))
      ext_master_zero_db = ext_master_max;
  }

  const char *master_ctl_name =
      snd_mixer_selem_get_name(ext_master_volume_ctl);
  assert(nullptr != master_ctl_name);
  if (ext_master_ctl_has_vol) {
    LogFormat("Using mixer element \"%s\" to control master volume control "
                  "on ALSA device \"%s\"",
              master_ctl_name, alsa_device_name);
  } else {
    assert(ext_master_ctl_has_switch);
    LogFormat("No usable mixer element for volume control found on ALSA "
                  "device \"%s\", but using on/off switch \"%s\"",
              alsa_device_name, master_ctl_name);
  }

  bool success = SetExternalVolumeNoLock(initial_vol_percent);

  if ((nullptr != pcm_volume_ctl) &&
      (pcm_volume_ctl != ext_master_volume_ctl)) {
    /* Ensure that the "PCM" volume is not muted. */

    if (snd_mixer_selem_has_playback_volume(pcm_volume_ctl)) {
      long pcm_value;

      if (0 != snd_mixer_selem_ask_playback_dB_vol(
                   pcm_volume_ctl, 0, 0, &pcm_value)) {
        long pcm_min;
        BOOST_VERIFY(0 == snd_mixer_selem_get_playback_volume_range(
                              pcm_volume_ctl,
                              &pcm_min,
                              &pcm_value));
      }

      LogFormat("Ensuring that PCM mixer element on ALSA device \"%s\" is "
                    "set to 100%%",
                alsa_device_name);

      BOOST_VERIFY(0 == snd_mixer_selem_set_playback_volume_all(
                            pcm_volume_ctl, pcm_value));
    }

    if (snd_mixer_selem_has_playback_switch(pcm_volume_ctl)) {
      LogFormat("Ensuring that PCM mixer element on ALSA device \"%s\" is "
                    "not muted",
                alsa_device_name);
      BOOST_VERIFY(0 == snd_mixer_selem_set_playback_switch_all(
                            pcm_volume_ctl,
                            1));
    }
  }

  return success;
}
#endif

void
VolumeController::SetInternalVolume(unsigned vol_percent
#ifdef HAVE_EXT_VOLUME_CONTROLLER
                                    , bool have_external_control
#endif
                                    )
{
  unsigned pcm_mixer_vol;

  assert(nullptr != pcm_mixer);

  constexpr unsigned max_pcm_mixer_safe_vol = PCMMixer::GetMaxSafeVolume();

  if (0 == vol_percent)
    pcm_mixer_vol = 0;
  else if (100 == vol_percent)
    pcm_mixer_vol = max_pcm_mixer_safe_vol;
  else if (GetMaxValue() == vol_percent)
    pcm_mixer_vol = 100;
  else if (vol_percent < 100)
    pcm_mixer_vol =
#if defined(HAVE_EXT_VOLUME_CONTROLLER)
        have_external_control ?
            max_pcm_mixer_safe_vol :
#endif
            max_pcm_mixer_safe_vol * vol_percent / 100;
  else
    pcm_mixer_vol = max_pcm_mixer_safe_vol +
        static_cast<unsigned>((100 - max_pcm_mixer_safe_vol) *
            static_cast<float>(vol_percent - 100) / (GetMaxValue() - 100));

  pcm_mixer->SetVolume(pcm_mixer_vol);
}

void
VolumeController::SetVolume(unsigned vol_percent)
{
#ifdef HAVE_EXT_VOLUME_CONTROLLER
  bool have_external_control = SetExternalVolume(vol_percent);
  SetInternalVolume(vol_percent, have_external_control);
#else
  SetInternalVolume(vol_percent);
#endif
}
