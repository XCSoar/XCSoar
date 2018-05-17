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

#include "Init.hpp"

#include <assert.h>
#include <stddef.h>
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
