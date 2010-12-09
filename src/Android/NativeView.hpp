/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include <jni.h>

class NativeView {
  JNIEnv *env;
  jobject obj;

  unsigned width, height;

  jmethodID swap_method, load_resource_texture_method;

public:
  NativeView(JNIEnv *_env, jobject _obj, unsigned _width, unsigned _height)
    :env(_env), obj(_obj), width(_width), height(_height) {
    jclass cls = env->FindClass("org/xcsoar/NativeView");
    swap_method = env->GetMethodID(cls, "swap", "()V");
    load_resource_texture_method = env->GetMethodID(cls, "loadResourceTexture",
                                                    "(Ljava/lang/String;[I)Z");
  }

  unsigned get_width() const { return width; }
  unsigned get_height() const { return height; }

  void swap() {
    env->CallVoidMethod(obj, swap_method);
  }

  bool loadResourceTexture(const char *name, jint *result) {
    jstring name2 = env->NewStringUTF(name);
    jintArray result2 = env->NewIntArray(3);

    bool success = env->CallBooleanMethod(obj, load_resource_texture_method,
                                          name2, result2);
    if (success)
      env->GetIntArrayRegion(result2, 0, 3, result);

    env->DeleteLocalRef(result2);
    env->DeleteLocalRef(name2);

    return success;
  }
};

#endif
