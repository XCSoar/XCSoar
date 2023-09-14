// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

#include <string_view>
#include <utility>

struct PixelSize;
class FontDescription;

class TextUtil : protected Java::GlobalObject {
  static JNIEnv *env;
  static jmethodID midTextUtil, midGetFontMetrics, midGetTextBounds;
  static jmethodID midGetTextTextureGL;

  unsigned height, ascent_height, capital_height;
  unsigned line_spacing;

  TextUtil(const Java::LocalObject &_obj) noexcept;

public:
  static void Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  static TextUtil *create(const FontDescription &d);

  [[gnu::pure]]
  PixelSize getTextBounds(std::string_view text) const noexcept;

  struct Texture {
    unsigned id;
    unsigned width, height;
    unsigned allocated_width, allocated_height;

    Texture(unsigned _id, unsigned _width, unsigned _height,
            unsigned _allocated_width, unsigned _allocated_height) noexcept
      :id(_id), width(_width), height(_height),
       allocated_width(_allocated_width),
       allocated_height(_allocated_height) {}
  };

  Texture getTextTextureGL(std::string_view text) const noexcept;

  unsigned get_height() const noexcept {
    return height;
  }

  unsigned get_ascent_height() const noexcept {
    return ascent_height;
  }

  unsigned get_capital_height() const noexcept {
    return capital_height;
  }

  unsigned GetLineSpacing() const noexcept {
    return line_spacing;
  }
};
