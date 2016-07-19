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

#ifndef XCSOAR_AUDIO_ALSA_ENV_HPP
#define XCSOAR_AUDIO_ALSA_ENV_HPP

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

#endif
