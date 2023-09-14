// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** \file
 *
 * Basic support for OpenSL/ES for native audio playback on Android.
 * Since OpenSL/ES was added with API version 9 (Android 2.3), we have
 * to load it dynamically to stay compatible with previous Android
 * versions.
 */

#pragma once

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
