// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextUtil.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "java/Exception.hxx"
#include "ui/dim/Size.hpp"
#include "Look/FontDescription.hpp"
#include "Asset.hpp"

#include <array>

JNIEnv *TextUtil::env;
static Java::TrivialClass cls;
jmethodID TextUtil::midTextUtil;
jmethodID TextUtil::midGetFontMetrics;
jmethodID TextUtil::midGetTextBounds;
jmethodID TextUtil::midGetTextTextureGL;

void
TextUtil::Initialise(JNIEnv *_env) noexcept
{
  env = _env;

  cls.Find(_env, "org/xcsoar/TextUtil");

  midTextUtil = _env->GetMethodID(cls, "<init>", "(IIIZ)V");
  midGetFontMetrics = _env->GetMethodID(cls, "getFontMetrics", "([I)V");
  midGetTextBounds = _env->GetMethodID(cls, "getTextBounds",
                                       "(Ljava/lang/String;)[I");
  midGetTextTextureGL = _env->GetMethodID(cls, "getTextTextureGL",
                                          "(Ljava/lang/String;)[I");
}

void
TextUtil::Deinitialise(JNIEnv *env) noexcept
{
  cls.Clear(env);
}

TextUtil::TextUtil(const Java::LocalObject &_obj) noexcept
  :Java::GlobalObject(_obj) {
  // get height, ascent_height and capital_height
  assert(midGetFontMetrics);

  auto &e = *_obj.GetEnv();

  Java::LocalRef<jintArray> metricsArray{&e, e.NewIntArray(5)};
  e.CallVoidMethod(Get(), midGetFontMetrics, metricsArray.Get());

  std::array<jint, 5> metrics;
  e.GetIntArrayRegion(metricsArray, 0, metrics.size(), metrics.data());
  height = metrics[0];
  ascent_height = metrics[1];
  capital_height = metrics[2];
  line_spacing = metrics[3];
}

TextUtil *
TextUtil::create(const FontDescription &d)
{
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
  auto &e = *env;
  Java::LocalObject localObject{&e,
    e.NewObject(cls, midTextUtil,
                paramStyle, paramTextSize,
                paint_flags, d.IsMonospace())};
  Java::RethrowException(&e);

  assert(localObject);

  return new TextUtil(localObject);
}

PixelSize
TextUtil::getTextBounds(std::string_view text) const noexcept
{
  jint extent[2];

  auto &e = *env;
  Java::String text2(&e, text);
  Java::LocalRef<jintArray> paramExtent{env,
    (jintArray)e.CallObjectMethod(Get(), midGetTextBounds, text2.Get())};
  if (!Java::DiscardException(&e)) {
    e.GetIntArrayRegion(paramExtent, 0, 2, extent);
  } else {
    /* Java exception has occurred; return zeroes */
    extent[0] = 0;
    extent[1] = 0;
  }

  return { extent[0], extent[1] };
}

TextUtil::Texture
TextUtil::getTextTextureGL(std::string_view text) const noexcept
{
  auto &e = *env;
  Java::String text2(&e, text);
  Java::LocalRef<jintArray> jresult{env,
    (jintArray)e.CallObjectMethod(Get(), midGetTextTextureGL,
                                  text2.Get())};
  jint result[5];
  if (!Java::DiscardException(&e) && jresult != nullptr) {
    e.GetIntArrayRegion(jresult, 0, 5, result);
  } else {
    result[0] = result[1] = result[2] = result[3] = result[4] = 0;
  }

  return Texture(result[0], result[1], result[2], result[3], result[4]);
}
