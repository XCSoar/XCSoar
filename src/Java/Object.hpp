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

#ifndef XCSOAR_JAVA_OBJECT_HPP
#define XCSOAR_JAVA_OBJECT_HPP

#include "Java/Global.hpp"
#include "Util/NonCopyable.hpp"

#include <jni.h>

namespace Java {
  class Object : private NonCopyable {
    jobject obj;

  public:
    /**
     * Constructs an uninitialized object.  The method set() must be
     * called before it is destructed.
     */
    Object() {}

    Object(JNIEnv *env, jobject _obj):obj(_obj) {
      env->NewGlobalRef(obj);
    }

    ~Object() {
      GetEnv()->DeleteGlobalRef(obj);
    }

    /**
     * Sets the object, ignoring the previous value.  This is only
     * allowed once after the default constructor was used.
     */
    void set(JNIEnv *env, jobject _obj) {
      obj = env->NewGlobalRef(_obj);
    }

    jobject get() const {
      return obj;
    }

    operator jobject() const {
      return obj;
    }

    void call_void(JNIEnv *env, const char *name) {
      jclass cls = env->GetObjectClass(obj);
      jmethodID mid = env->GetMethodID(cls, name, "()V");
      env->CallVoidMethod(obj, mid);
      env->DeleteLocalRef(cls);
    }

    void call_void(const char *name) {
      call_void(GetEnv(), name);
    }
  };
}

#endif
