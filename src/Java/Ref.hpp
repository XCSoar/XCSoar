/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_JAVA_GLOBAL_REF_HPP
#define XCSOAR_JAVA_GLOBAL_REF_HPP

#include "Java/Global.hpp"
#include "Util/NonCopyable.hpp"

#include <assert.h>
#include <jni.h>

namespace Java {
  /**
   * Hold a local reference on a JNI object.
   */
  template<typename T>
  class LocalRef : private NonCopyable {
    JNIEnv *const env;
    const T value;

  public:
    /**
     * The local reference is obtained by the caller.
     */
    LocalRef(JNIEnv *_env, T _value):env(_env), value(_value) {
      assert(env != NULL);
      assert(value != NULL);
    }

    ~LocalRef() {
      env->DeleteLocalRef(value);
    }

    T get() const {
      return value;
    }

    operator T() const {
      return value;
    }
  };

  /**
   * Hold a global reference on a JNI object.
   */
  template<typename T>
  class GlobalRef : private NonCopyable {
    T value;

  public:
    /**
     * Constructs an uninitialized object.  The method set() must be
     * called before it is destructed.
     */
    GlobalRef() {}

    GlobalRef(JNIEnv *env, T _value):value(_value) {
      assert(env != NULL);
      assert(value != NULL);

      env->NewGlobalRef(value);
    }

    ~GlobalRef() {
      GetEnv()->DeleteGlobalRef(value);
    }

    /**
     * Sets the object, ignoring the previous value.  This is only
     * allowed once after the default constructor was used.
     */
    void set(JNIEnv *env, T _value) {
      assert(_value != NULL);

      value = env->NewGlobalRef(_value);
    }

    T get() const {
      return value;
    }

    operator T() const {
      return value;
    }
  };
}

#endif
