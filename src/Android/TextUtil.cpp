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

#include "TextUtil.hpp"
#include "Java/Class.hxx"
#include "Java/String.hxx"
#include "Java/Exception.hxx"
#include "Screen/Point.hpp"
#include "Look/FontDescription.hpp"
#include "Asset.hpp"

JNIEnv *TextUtil::env;
static Java::TrivialClass cls;
jmethodID TextUtil::midTextUtil;
jmethodID TextUtil::midGetFontMetrics;
jmethodID TextUtil::midGetTextBounds;
jmethodID TextUtil::midGetTextTextureGL;

void
TextUtil::Initialise(JNIEnv *_env)
{
  env = _env;

  cls.Find(env, "org/xcsoar/TextUtil");

  midTextUtil = env->GetMethodID(cls, "<init>", "(IIIZ)V");
  midGetFontMetrics = env->GetMethodID(cls, "getFontMetrics", "([I)V");
  midGetTextBounds = env->GetMethodID(cls, "getTextBounds",
                                      "(Ljava/lang/String;)[I");
  midGetTextTextureGL = env->GetMethodID(cls, "getTextTextureGL",
                                         "(Ljava/lang/String;)[I");
}

void
TextUtil::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

TextUtil::TextUtil(jobject _obj)
  :Java::GlobalObject(env, _obj) {
  // get height, ascent_height and capital_height
  assert(midGetFontMetrics);
  jintArray metricsArray = env->NewIntArray(5);
  env->CallVoidMethod(Get(), midGetFontMetrics, metricsArray);

  jint metrics[5];
  env->GetIntArrayRegion(metricsArray, 0, 5, metrics);
  height = metrics[0];
  style = metrics[1];
  ascent_height = metrics[2];
  capital_height = metrics[3];
  line_spacing = metrics[4];

  // free local references
  env->DeleteLocalRef(metricsArray);
}

TextUtil *
TextUtil::create(const FontDescription &d)
{
  jobject localObject;
  jint paramStyle, paramTextSize;

  paramStyle = 0;
  if (d.IsBold())
    paramStyle |= 1;
  if (d.IsItalic())
    paramStyle |= 2;
  paramTextSize = d.GetHeight();

  int paint_flags = 0;
  if (!IsDithered())
    /* 1 = Paint.ANTI_ALIAS_FLAG */
    paint_flags |= 1;

  // construct org.xcsoar.TextUtil object
  localObject = env->NewObject(cls, midTextUtil,
                               paramStyle, paramTextSize,
                               paint_flags, d.IsMonospace());
  if (!localObject)
    return nullptr;

  TextUtil *tu = new TextUtil(localObject);

  env->DeleteLocalRef(localObject);

  return tu;
}

PixelSize
TextUtil::getTextBounds(const char *text) const
{
  jint extent[2];

  Java::String text2(env, text);
  jintArray paramExtent = (jintArray)
    env->CallObjectMethod(Get(), midGetTextBounds,
                          text2.Get());
  if (!Java::DiscardException(env)) {
    env->GetIntArrayRegion(paramExtent, 0, 2, extent);
    env->DeleteLocalRef(paramExtent);
  } else {
    /* Java exception has occurred; return zeroes */
    extent[0] = 0;
    extent[1] = 0;
  }

  return { extent[0], extent[1] };
}

TextUtil::Texture
TextUtil::getTextTextureGL(const char *text) const
{
  Java::String text2(env, text);
  jintArray jresult = (jintArray)
    env->CallObjectMethod(Get(), midGetTextTextureGL,
                          text2.Get());
  jint result[5];
  if (!Java::DiscardException(env) && jresult != nullptr) {
    env->GetIntArrayRegion(jresult, 0, 5, result);
    env->DeleteLocalRef(jresult);
  } else {
    result[0] = result[1] = result[2] = result[3] = result[4] = 0;
  }

  return Texture(result[0], result[1], result[2], result[3], result[4]);
}
