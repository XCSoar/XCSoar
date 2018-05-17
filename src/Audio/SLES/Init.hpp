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

/** \file
 *
 * Basic support for OpenSL/ES for native audio playback on Android.
 * Since OpenSL/ES was added with API version 9 (Android 2.3), we have
 * to load it dynamically to stay compatible with previous Android
 * versions.
 */

#ifndef XCSOAR_AUDIO_SLES_INIT_HPP
#define XCSOAR_AUDIO_SLES_INIT_HPP

#include <SLES/OpenSLES.h>

namespace SLES {
  extern const SLInterfaceID *IID_ENGINE, *IID_PLAY,
    *IID_ANDROIDSIMPLEBUFFERQUEUE;

  /**
   * Attempt to load libOpenSLES.so with dlopen(), and look up the SLES
   * functions which we need.
   *
   * @return true if libOpenSLES.so is available
   */
  bool
  Initialise();

  /**
   * Wrapper for slCreateEngine().  Returns an error when Initialise()
   * has failed.
   */
  SLresult
  CreateEngine(SLObjectItf *pEngine, SLuint32 numOptions,
               const SLEngineOption *pEngineOptions,
               SLuint32 numInterfaces, const SLInterfaceID *pInterfaceIds,
               const SLboolean *pInterfaceRequired);
}

#endif
