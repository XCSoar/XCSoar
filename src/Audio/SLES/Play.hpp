// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <SLES/OpenSLES.h>

namespace SLES {
  /**
   * OO wrapper for an OpenSL/ES SLPlayItf variable.
   */
  class Play {
    SLPlayItf play;

  public:
    Play() = default;
    explicit Play(SLPlayItf _play):play(_play) {}

    SLresult SetPlayState(SLuint32 state) {
      return (*play)->SetPlayState(play, state);
    }
  };
}
