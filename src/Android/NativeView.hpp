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

#ifndef XCSOAR_ANDROID_NATIVE_VIEW_HPP
#define XCSOAR_ANDROID_NATIVE_VIEW_HPP

#include "Java/Object.hpp"
#include "Java/Class.hpp"
#include "Java/String.hpp"

#include <assert.h>

class NativeView {
  JNIEnv *env;
  Java::Object obj;

  unsigned width, height;

  jmethodID init_surface_method, deinit_surface_method;
  jmethodID setRequestedOrientationID;
  jmethodID swap_method, load_resource_texture_method;

public:
  /**
   * @see http://developer.android.com/reference/android/R.attr.html#screenOrientation
   */
  enum screen_orientation {
    SCREEN_ORIENTATION_UNSPECIFIED = -1,
    SCREEN_ORIENTATION_LANDSCAPE = 0,
    SCREEN_ORIENTATION_PORTRAIT = 1,
    SCREEN_ORIENTATION_USER = 2,
    SCREEN_ORIENTATION_BEHIND = 3,
    SCREEN_ORIENTATION_SENSOR = 4,
    SCREEN_ORIENTATION_NOSENSOR = 5,
    SCREEN_ORIENTATION_REVERSE_LANDSCAPE = 8,
    SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9,
  };

  NativeView(JNIEnv *_env, jobject _obj,
             unsigned _width, unsigned _height)
    :env(_env), obj(env, _obj),
     width(_width), height(_height) {
    Java::Class cls(env, "org/xcsoar/NativeView");
    init_surface_method = env->GetMethodID(cls, "initSurface", "()Z");
    deinit_surface_method = env->GetMethodID(cls, "deinitSurface", "()V");
    setRequestedOrientationID =
      env->GetMethodID(cls, "setRequestedOrientation", "(I)Z");
    swap_method = env->GetMethodID(cls, "swap", "()V");
    load_resource_texture_method = env->GetMethodID(cls, "loadResourceTexture",
                                                    "(Ljava/lang/String;[I)Z");
  }

  unsigned get_width() const { return width; }
  unsigned get_height() const { return height; }

  void SetSize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
  }

  bool initSurface() {
    return env->CallBooleanMethod(obj, init_surface_method);
  }

  void deinitSurface() {
    env->CallVoidMethod(obj, deinit_surface_method);
  }

  bool setRequestedOrientation(screen_orientation so) {
    return env->CallBooleanMethod(obj, setRequestedOrientationID, (jint)so);
  }

  void swap() {
    env->CallVoidMethod(obj, swap_method);
  }

  bool loadResourceTexture(const char *name, jint *result) {
    Java::String name2(env, name);
    jintArray result2 = env->NewIntArray(3);

    bool success = env->CallBooleanMethod(obj, load_resource_texture_method,
                                          name2.get(), result2);
    if (success)
      env->GetIntArrayRegion(result2, 0, 3, result);

    env->DeleteLocalRef(result2);

    return success;
  }
};

#endif
