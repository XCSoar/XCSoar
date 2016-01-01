/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "NativeView.hpp"

Java::TrivialClass NativeView::cls;
jfieldID NativeView::textureNonPowerOfTwo_field;
jmethodID NativeView::init_surface_method, NativeView::deinit_surface_method;
jmethodID NativeView::setRequestedOrientationID;
jmethodID NativeView::swap_method;
jmethodID NativeView::loadResourceBitmap_method;
jmethodID NativeView::loadFileBitmap_method;
jmethodID NativeView::bitmapToTexture_method;
jmethodID NativeView::open_file_method;
jmethodID NativeView::getNetState_method;

void
NativeView::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativeView");

  textureNonPowerOfTwo_field =
    env->GetStaticFieldID(cls, "textureNonPowerOfTwo", "Z");
  init_surface_method = env->GetMethodID(cls, "initSurface", "()Z");
  deinit_surface_method = env->GetMethodID(cls, "deinitSurface", "()V");
  setRequestedOrientationID =
    env->GetMethodID(cls, "setRequestedOrientation", "(I)Z");
  swap_method = env->GetMethodID(cls, "swap", "()V");

  loadResourceBitmap_method = env->GetMethodID(cls, "loadResourceBitmap",
                                               "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
  loadFileBitmap_method = env->GetMethodID(cls, "loadFileBitmap",
                                           "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
  bitmapToTexture_method = env->GetMethodID(cls, "bitmapToTexture",
                                            "(Landroid/graphics/Bitmap;Z[I)Z");

  open_file_method = env->GetMethodID(cls, "openFile",
                                      "(Ljava/lang/String;)V");

  getNetState_method = env->GetMethodID(cls, "getNetState", "()I");
}

void
NativeView::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}
