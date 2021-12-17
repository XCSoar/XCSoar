/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "java/Object.hxx"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "system/Path.hpp"

#ifndef NO_SCREEN
#include "ui/dim/Size.hpp"
#endif

#include <cassert>

class NativeView {
  JNIEnv *env;
  Java::GlobalObject obj;

  unsigned width, height;
  char product[20];

  static Java::TrivialClass cls;
  static jfieldID textureNonPowerOfTwo_field;
  static jmethodID init_surface_method, deinit_surface_method;
  static jmethodID acquireWakeLock_method;
  static jmethodID setFullScreen_method;
  static jmethodID setRequestedOrientationID;
  static jmethodID loadResourceBitmap_method;
  static jmethodID loadFileBitmap_method;
  static jmethodID bitmapToTexture_method;
  static jmethodID shareText_method;
  static jmethodID openWaypointFile_method;
  static jmethodID getNetState_method;

  static Java::TrivialClass clsBitmap;
  static jmethodID createBitmap_method;

  static Java::TrivialClass clsBitmapConfig;
  static jmethodID bitmapConfigValueOf_method;
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
  };

  static void Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  NativeView(JNIEnv *_env, jobject _obj, unsigned _width, unsigned _height,
             unsigned _xdpi, unsigned _ydpi,
             jstring _product) noexcept;

#ifndef NO_SCREEN
  PixelSize GetSize() const {
    return { width, height };
  }
#endif

  void SetSize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
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

  void AcquireWakeLock() const noexcept {
    return env->CallVoidMethod(obj, acquireWakeLock_method);
  }

  void SetFullScreen(bool full_screen) const noexcept {
    return env->CallVoidMethod(obj, setFullScreen_method, full_screen);
  }

  bool setRequestedOrientation(ScreenOrientation so) {
    return env->CallBooleanMethod(obj, setRequestedOrientationID, (jint)so);
  }

  Java::LocalObject loadResourceBitmap(const char *name) {
    Java::String name2(env, name);
    return {env,
      env->CallObjectMethod(obj, loadResourceBitmap_method, name2.Get())};
  }

  Java::LocalObject loadFileTiff(Path path);

  Java::LocalObject loadFileBitmap(Path path);

  bool bitmapToTexture(jobject bmp, bool alpha, jint *result) {
    Java::LocalRef<jintArray> result2{env, env->NewIntArray(5)};

    bool success = env->CallBooleanMethod(obj, bitmapToTexture_method,
                                          bmp, alpha, result2.Get());
    if (success)
      env->GetIntArrayRegion(result2, 0, 5, result);

    return success;
  }

  void SetTexturePowerOfTwo(bool value) {
    env->SetStaticBooleanField(cls, textureNonPowerOfTwo_field, value);
  }

  /**
   * Deliver plain text data to somebody; the user will be asked to
   * pick a recipient.
   */
  void ShareText(const char *text) noexcept;

  void OpenWaypointFile(unsigned id, const char *filename) {
    env->CallVoidMethod(obj, openWaypointFile_method, id,
                        Java::String(env, filename).Get());
  }

  [[gnu::pure]]
  int getNetState() const {
    return env->CallIntMethod(obj, getNetState_method);
  }
};

#endif
