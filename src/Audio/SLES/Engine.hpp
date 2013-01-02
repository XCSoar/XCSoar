/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_AUDIO_SLES_ENGINE_HPP
#define XCSOAR_AUDIO_SLES_ENGINE_HPP

#include <SLES/OpenSLES.h>

namespace SLES {
  /**
   * OO wrapper for an OpenSL/ES SLEngineItf variable.
   */
  class Engine {
    SLEngineItf engine;

  public:
    Engine() = default;
    explicit Engine(SLEngineItf _engine):engine(_engine) {}

    SLresult CreateAudioPlayer(SLObjectItf *pPlayer,
                               SLDataSource *pAudioSrc, SLDataSink *pAudioSnk,
                               SLuint32 numInterfaces,
                               const SLInterfaceID *pInterfaceIds,
                               const SLboolean *pInterfaceRequired) {
      return (*engine)->CreateAudioPlayer(engine, pPlayer,
                                          pAudioSrc, pAudioSnk,
                                          numInterfaces, pInterfaceIds,
                                          pInterfaceRequired);
    }

    SLresult CreateOutputMix(SLObjectItf *pMix,
                             SLuint32 numInterfaces,
                             const SLInterfaceID *pInterfaceIds,
                             const SLboolean *pInterfaceRequired) {
      return (*engine)->CreateOutputMix(engine, pMix,
                                        numInterfaces, pInterfaceIds,
                                        pInterfaceRequired);
    }
  };
}

#endif
