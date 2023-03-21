// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

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
