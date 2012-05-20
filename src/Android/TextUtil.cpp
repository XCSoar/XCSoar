/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Android/TextUtil.hpp"
#include "Java/Class.hpp"
#include "Java/String.hpp"
#include "Java/Exception.hpp"
#include "Screen/Point.hpp"

JNIEnv *TextUtil::env(NULL);
jmethodID TextUtil::midTextUtil(NULL);
jmethodID TextUtil::midGetFontMetrics(NULL);
jmethodID TextUtil::midGetTextBounds(NULL);
jmethodID TextUtil::midGetTextTextureGL(NULL);

TextUtil::TextUtil(jobject _obj, jclass textUtilClass)
  :Java::Object(env, _obj) {
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
TextUtil::create(const char *facename, int height, bool bold, bool italic)
{
  jobject localObject;
  jint paramStyle, paramTextSize;

  if (env == NULL) {
    // initialize static jvm
    env = Java::GetEnv();
  }

  Java::Class textUtilClass(env, "org/xcsoar/TextUtil");

  if (midTextUtil == NULL) {
    // initialize static method ID's once
    midTextUtil         = env->GetMethodID(textUtilClass, "<init>",
                                           "(Ljava/lang/String;II)V");
    midGetFontMetrics   = env->GetMethodID(textUtilClass, "getFontMetrics",
                                           "([I)V");
    midGetTextBounds    = env->GetMethodID(textUtilClass, "getTextBounds",
                                           "(Ljava/lang/String;)[I");
    midGetTextTextureGL = env->GetMethodID(textUtilClass, "getTextTextureGL",
                                           "(Ljava/lang/String;)[I");
  }

  Java::String paramFamilyName(env, facename);
  paramStyle = 0;
  if (bold)
    paramStyle |= 1;
  if (italic)
    paramStyle |= 2;
  paramTextSize = height;

  // construct org.xcsoar.TextUtil object
  localObject = env->NewObject(textUtilClass, midTextUtil,
                               paramFamilyName.Get(),
                               paramStyle, paramTextSize);
  if (!localObject)
    return NULL;

  TextUtil *tu = new TextUtil(localObject, textUtilClass);

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

  PixelSize size = { PixelScalar(extent[0]),
                     PixelScalar(extent[1]) };
  return size;
}

TextUtil::Texture
TextUtil::getTextTextureGL(const char *text) const
{
  Java::String text2(env, text);
  jintArray jresult = (jintArray)
    env->CallObjectMethod(Get(), midGetTextTextureGL,
                          text2.Get());
  jint result[3];
  if (!Java::DiscardException(env)) {
    env->GetIntArrayRegion(jresult, 0, 3, result);
    env->DeleteLocalRef(jresult);
  } else {
    result[0] = result[1] = result[2] = 0;
  }

  return Texture(result[0], result[1], result[2]);
}
