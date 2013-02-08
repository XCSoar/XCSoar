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

#ifndef XCSOAR_ANDROID_NATIVE_VIEW_HPP
#define XCSOAR_ANDROID_NATIVE_VIEW_HPP

#include "Java/Object.hpp"
#include "Java/Class.hpp"
#include "Java/String.hpp"
#include "Screen/Point.hpp"

#include <assert.h>

class NativeView {
  JNIEnv *env;
  Java::Object obj;

  unsigned width, height;
  unsigned xdpi, ydpi;
  unsigned sdk_version;
  char product[20];

  jmethodID init_surface_method, deinit_surface_method;
  jmethodID setRequestedOrientationID;
  jmethodID swap_method, load_resource_texture_method;
  jmethodID load_file_texture_method, open_file_method;

public:
  /**
   * @see http://developer.android.com/reference/android/R.attr.html#screenOrientation
   */
  enum class ScreenOrientation {
    // API level 1
    UNSPECIFIED = -1,
    LANDSCAPE = 0,
    PORTRAIT = 1,
    USER = 2,
    BEHIND = 3,
    SENSOR = 4,
    NOSENSOR = 5,
    // API level 9
    // see http://developer.android.com/reference/android/content/pm/ActivityInfo.html#SCREEN_ORIENTATION_REVERSE_LANDSCAPE
    REVERSE_LANDSCAPE = 8,
    REVERSE_PORTRAIT = 9,
    // HACK for Galaxy Tab (FROYO = 2.2 = API level 8)
    REVERSE_LANDSCAPE_GT = 7,
    REVERSE_PORTRAIT_GT = 8,
  };

  NativeView(JNIEnv *_env, jobject _obj, unsigned _width, unsigned _height,
             unsigned _xdpi, unsigned _ydpi,
             unsigned _sdk_version, jstring _product)
    :env(_env), obj(env, _obj),
     width(_width), height(_height),
     xdpi(_xdpi), ydpi(_ydpi),
     sdk_version(_sdk_version) {
    Java::String::CopyTo(env, _product, product, sizeof(product));
    Java::Class cls(env, "org/xcsoarte/NativeView");
    init_surface_method = env->GetMethodID(cls, "initSurface", "()Z");
    deinit_surface_method = env->GetMethodID(cls, "deinitSurface", "()V");
    setRequestedOrientationID =
      env->GetMethodID(cls, "setRequestedOrientation", "(I)Z");
    swap_method = env->GetMethodID(cls, "swap", "()V");
    load_resource_texture_method = env->GetMethodID(cls, "loadResourceTexture",
                                                    "(Ljava/lang/String;[I)Z");
    load_file_texture_method = env->GetMethodID(cls, "loadFileTexture",
                                                "(Ljava/lang/String;[I)Z");
    open_file_method = env->GetMethodID(cls, "openFile",
                                        "(Ljava/lang/String;)V");
  }

  unsigned GetWidth() const { return width; }
  unsigned GetHeight() const { return height; }

  PixelSize GetSize() const {
    return { width, height };
  }

  unsigned GetXDPI() const {
    return xdpi;
  }

  unsigned GetYDPI() const {
    return ydpi;
  }

  void SetSize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
  }

  int GetAPILevel() {
    return sdk_version;
  }

  const char *GetProduct() {
    return product;
  }

  bool initSurface() {
    return env->CallBooleanMethod(obj, init_surface_method);
  }

  void deinitSurface() {
    env->CallVoidMethod(obj, deinit_surface_method);
  }

  bool setRequestedOrientation(ScreenOrientation so) {
    return env->CallBooleanMethod(obj, setRequestedOrientationID, (jint)so);
  }

  void swap() {
    env->CallVoidMethod(obj, swap_method);
  }

  bool loadResourceTexture(const char *name, jint *result) {
    Java::String name2(env, name);
    jintArray result2 = env->NewIntArray(3);

    bool success = env->CallBooleanMethod(obj, load_resource_texture_method,
                                          name2.Get(), result2);
    if (success)
      env->GetIntArrayRegion(result2, 0, 3, result);

    env->DeleteLocalRef(result2);

    return success;
  }

  bool loadFileTexture(const char *pathName, jint *result) {
    Java::String pathName2(env, pathName);
    jintArray result2 = env->NewIntArray(3);

    bool success = env->CallBooleanMethod(obj, load_file_texture_method,
                                          pathName2.Get(), result2);
    if (success)
      env->GetIntArrayRegion(result2, 0, 3, result);

    env->DeleteLocalRef(result2);

    return success;
  }

  void SetTexturePowerOfTwo(bool value) {
    Java::Class cls(env, env->GetObjectClass(obj));
    jfieldID id = env->GetStaticFieldID(cls, "textureNonPowerOfTwo", "Z");
    assert(id);

    env->SetStaticBooleanField(cls, id, value);
  }

  void openFile(const char *pathName) {
    Java::String pathName2(env, pathName);
    env->CallVoidMethod(obj, open_file_method, pathName2.Get());
  }
};

#endif
