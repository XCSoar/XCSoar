/*
 * Copyright (C) 2010-2011 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
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

      value = (T)env->NewGlobalRef(value);
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

      value = (T)env->NewGlobalRef(_value);
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
