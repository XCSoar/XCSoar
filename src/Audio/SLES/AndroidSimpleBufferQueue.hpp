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

#ifndef XCSOAR_AUDIO_SLES_ANDROID_SIMPLE_BUFFER_QUEUE_HPP
#define XCSOAR_AUDIO_SLES_ANDROID_SIMPLE_BUFFER_QUEUE_HPP

#include <SLES/OpenSLES_Android.h>

namespace SLES {
  /**
   * OO wrapper for an OpenSL/ES SLAndroidSimpleBufferQueueItf
   * variable.
   */
  class AndroidSimpleBufferQueue {
    SLAndroidSimpleBufferQueueItf queue;

  public:
    AndroidSimpleBufferQueue() = default;
    explicit AndroidSimpleBufferQueue(SLAndroidSimpleBufferQueueItf _queue)
      :queue(_queue) {}

    SLresult Enqueue(const void *pBuffer, SLuint32 size) {
      return (*queue)->Enqueue(queue, pBuffer, size);
    }

    SLresult Clear() {
      return (*queue)->Clear(queue);
    }

    SLresult GetState(SLAndroidSimpleBufferQueueState *pState) {
      return (*queue)->GetState(queue, pState);
    }

    SLresult RegisterCallback(slAndroidSimpleBufferQueueCallback callback,
                              void *pContext) {
      return (*queue)->RegisterCallback(queue, callback, pContext);
    }
  };
}

#endif
