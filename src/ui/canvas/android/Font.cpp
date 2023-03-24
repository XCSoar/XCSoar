// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Font.hpp"
#include "Screen/Debug.hpp"
#include "Look/FontDescription.hpp"
#include "java/Global.hxx"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "Android/TextUtil.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

/*
 * create a new instance of org.xcsoar.TextUtil and store it with a global
 * reference in text_util_object member.
 */
void
Font::Load(const FontDescription &d)
{
  assert(IsScreenInitialized());

  delete text_util_object;
  text_util_object = TextUtil::create(d);
  assert(text_util_object);

  this->height = text_util_object->get_height();
  ascent_height = text_util_object->get_ascent_height();
  capital_height = text_util_object->get_capital_height();
  line_spacing = text_util_object->GetLineSpacing();
}

void
Font::Destroy() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

  delete text_util_object;
  text_util_object = nullptr;
}

PixelSize
Font::TextSize(tstring_view text) const noexcept
{
  if (text_util_object == nullptr) {
    PixelSize empty = { 0, 0 };
    return empty;
  }

  return text_util_object->getTextBounds(text);
}

int
Font::TextTextureGL(tstring_view text, PixelSize &size,
                    PixelSize &allocated_size) const noexcept
{
  if (!text_util_object)
    return 0;

  if (text.empty())
    return 0;

  const TextUtil::Texture texture =
    text_util_object->getTextTextureGL(text);
  size.width = texture.width;
  size.height = texture.height;
  allocated_size.width = texture.allocated_width;
  allocated_size.height = texture.allocated_height;
  return texture.id;
}
