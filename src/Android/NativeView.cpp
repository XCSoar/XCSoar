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

#include "util/StringCompare.hxx"
#include "system/Path.hpp"
#include "ui/canvas/custom/LibTiff.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "NativeView.hpp"
#include "Hardware/DisplayDPI.hpp"

#include <tchar.h>

Java::TrivialClass NativeView::cls;
jfieldID NativeView::textureNonPowerOfTwo_field;
jmethodID NativeView::init_surface_method, NativeView::deinit_surface_method;
jmethodID NativeView::acquireWakeLock_method;
jmethodID NativeView::setFullScreen_method;
jmethodID NativeView::setRequestedOrientationID;
jmethodID NativeView::loadResourceBitmap_method;
jmethodID NativeView::loadFileBitmap_method;
jmethodID NativeView::bitmapToTexture_method;
jmethodID NativeView::shareText_method;
jmethodID NativeView::openWaypointFile_method;
jmethodID NativeView::getNetState_method;

Java::TrivialClass NativeView::clsBitmap;
jmethodID NativeView::createBitmap_method;

Java::TrivialClass NativeView::clsBitmapConfig;
jmethodID NativeView::bitmapConfigValueOf_method;

void
NativeView::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativeView");

  textureNonPowerOfTwo_field =
    env->GetStaticFieldID(cls, "textureNonPowerOfTwo", "Z");
  init_surface_method = env->GetMethodID(cls, "initSurface", "()Z");
  deinit_surface_method = env->GetMethodID(cls, "deinitSurface", "()V");

  acquireWakeLock_method = env->GetMethodID(cls, "acquireWakeLock", "()V");

  setFullScreen_method =
    env->GetMethodID(cls, "setFullScreen", "(Z)V");

  setRequestedOrientationID =
    env->GetMethodID(cls, "setRequestedOrientation", "(I)Z");

  loadResourceBitmap_method = env->GetMethodID(cls, "loadResourceBitmap",
                                               "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
  loadFileBitmap_method = env->GetMethodID(cls, "loadFileBitmap",
                                           "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
  bitmapToTexture_method = env->GetMethodID(cls, "bitmapToTexture",
                                            "(Landroid/graphics/Bitmap;Z[I)Z");

  shareText_method = env->GetMethodID(cls, "shareText",
                                          "(Ljava/lang/String;)V");

  openWaypointFile_method =
    env->GetMethodID(cls, "openWaypointFile",
                     "(ILjava/lang/String;)V");

  getNetState_method = env->GetMethodID(cls, "getNetState", "()I");

  clsBitmap.Find(env, "android/graphics/Bitmap");
  createBitmap_method = env->GetStaticMethodID(
    clsBitmap, "createBitmap",
    "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

  clsBitmapConfig.Find(env, "android/graphics/Bitmap$Config");
  bitmapConfigValueOf_method = env->GetStaticMethodID(
    clsBitmapConfig, "valueOf", "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
}

void
NativeView::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

NativeView::NativeView(JNIEnv *_env, jobject _obj,
                       unsigned _width, unsigned _height,
                       unsigned _xdpi, unsigned _ydpi,
                       jstring _product) noexcept
  :env(_env), obj(env, _obj),
   width(_width), height(_height)
{
  Java::String::CopyTo(env, _product, product, sizeof(product));

  Display::ProvideDPI(_xdpi, _ydpi);
}

static void
ConvertABGRToARGB(UncompressedImage &image)
{
  // TODO: Get rid of the const_cast! Maybe move this to UncompressedImage?
  uint8_t *data = static_cast<uint8_t *>(const_cast<void *>(image.GetData()));
  const uint8_t *data_end = data + image.GetWidth() * image.GetHeight() * 4;
  for (uint8_t *p = data; p != data_end; p +=4) {
    std::swap(p[0], p[2]);
  }
}

Java::LocalObject
NativeView::loadFileTiff(Path path)
{
  UncompressedImage image = LoadTiff(path);

  // create a Bitmap.Config enum
  Java::String config_name(env, "ARGB_8888");
  Java::LocalObject bitmap_config{env,
    env->CallStaticObjectMethod(clsBitmapConfig, bitmapConfigValueOf_method,
                                config_name.Get())};

  // convert ABGR to ARGB
  // TODO: I am not sure if this conversion depends on endianess. So
  //       it might be wrong for Intel CPUs?!
  ConvertABGRToARGB(image);

  // create int array
  unsigned size = image.GetWidth() * image.GetHeight();
  Java::LocalRef<jintArray> intArray{env, env->NewIntArray(size)};
  env->SetIntArrayRegion(intArray, 0, size, static_cast<const jint*>(image.GetData()));

  // call Bitmap.createBitmap()
  return {env,
    env->CallStaticObjectMethod(clsBitmap, createBitmap_method,
                                intArray.Get(),
                                image.GetWidth(), image.GetHeight(),
                                bitmap_config.Get())};
}

Java::LocalObject
NativeView::loadFileBitmap(Path path)
{
  Java::String path2(env, path.c_str());
  return {env, env->CallObjectMethod(obj, loadFileBitmap_method, path2.Get())};
}

void
NativeView::ShareText(const char *text) noexcept
{
  env->CallVoidMethod(obj, shareText_method,
                      Java::String{env, text}.Get());
}
