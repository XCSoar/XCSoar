/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

  static TextUtil *create(const FontDescription &d) noexcept;

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
