// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

#include <string_view>

class Font;

#ifdef ENABLE_OPENGL
class GLTexture;
#endif

namespace TextCache {

#ifdef ENABLE_OPENGL
typedef GLTexture *Result;
#else
struct Result {
  const void *data;
  unsigned pitch;
  PixelSize size;

  constexpr Result(const void *_data, unsigned _pitch, PixelSize _size) noexcept
    :data(_data), pitch(_pitch), size(_size) {}

  constexpr Result(std::nullptr_t) noexcept
    :data{}, pitch{}, size{} {}

  constexpr operator bool() const noexcept {
    return data != nullptr;
  }
};
#endif

[[gnu::pure]]
PixelSize
GetSize(const Font &font, std::string_view text) noexcept;

[[gnu::pure]]
PixelSize
LookupSize(const Font &font, std::string_view text) noexcept;

[[gnu::pure]]
Result
Get(const Font &font, std::string_view text) noexcept;

void
Flush() noexcept;

} //namespace TextCache
