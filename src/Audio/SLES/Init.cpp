// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"

#include <cassert>
#include <cstddef>
#include <dlfcn.h>

typedef SLresult (*T_slCreateEngine)(SLObjectItf *pEngine,
                                     SLuint32 numOptions,
                                     const SLEngineOption *pEngineOptions,
                                     SLuint32 numInterfaces,
                                     const SLInterfaceID *pInterfaceIds,
                                     const SLboolean *pInterfaceRequired);

static T_slCreateEngine _slCreateEngine;

namespace SLES {
  const SLInterfaceID *IID_ENGINE, *IID_PLAY,
    *IID_ANDROIDSIMPLEBUFFERQUEUE;
}

bool
SLES::Initialise()
{
  void *sles = dlopen("libOpenSLES.so", RTLD_NOW);
  if (sles == nullptr)
    return false;

  IID_ENGINE = (const SLInterfaceID *)dlsym(sles, "SL_IID_ENGINE");
  IID_PLAY = (const SLInterfaceID *)dlsym(sles, "SL_IID_PLAY");
  IID_ANDROIDSIMPLEBUFFERQUEUE = (const SLInterfaceID *)
    dlsym(sles, "SL_IID_ANDROIDSIMPLEBUFFERQUEUE");
  if (IID_ENGINE == nullptr || IID_PLAY == nullptr ||
      IID_ANDROIDSIMPLEBUFFERQUEUE == nullptr)
    return false;

  _slCreateEngine = (T_slCreateEngine)dlsym(sles, "slCreateEngine");
  if (_slCreateEngine == nullptr)
    return false;

  return true;
}

SLresult
SLES::CreateEngine(SLObjectItf *pEngine, SLuint32 numOptions,
                   const SLEngineOption *pEngineOptions,
                   SLuint32 numInterfaces, const SLInterfaceID *pInterfaceIds,
                   const SLboolean *pInterfaceRequired)
{
  if (_slCreateEngine == nullptr)
    /* Initialise() has failed, bail out */
    return SL_RESULT_FEATURE_UNSUPPORTED;

  return _slCreateEngine(pEngine, numOptions, pEngineOptions,
                         numInterfaces, pInterfaceIds, pInterfaceRequired);
}
