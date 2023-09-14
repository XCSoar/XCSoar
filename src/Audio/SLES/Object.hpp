// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <SLES/OpenSLES.h>

namespace SLES {
  /**
   * OO wrapper for an OpenSL/ES SLObjectItf variable.
   */
  class Object {
    SLObjectItf object;

  public:
    Object() = default;
    explicit Object(SLObjectItf _object):object(_object) {}

    operator SLObjectItf() {
      return object;
    }

    SLresult Realize(bool async) {
      return (*object)->Realize(object, async);
    }

    void Destroy() {
      (*object)->Destroy(object);
    }

    SLresult GetInterface(const SLInterfaceID iid, void *pInterface) {
      return (*object)->GetInterface(object, iid, pInterface);
    }
  };
}
