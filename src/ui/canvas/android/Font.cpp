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

#include "ui/canvas/Font.hpp"
#include "Screen/Debug.hpp"
#include "Look/FontDescription.hpp"
#include "java/Global.hxx"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "Android/TextUtil.hpp"
#include "util/StringCompare.hxx"
#include "util/TStringView.hxx"

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
Font::TextSize(TStringView text) const noexcept
{
  if (text_util_object == nullptr) {
    PixelSize empty = { 0, 0 };
    return empty;
  }

  return text_util_object->getTextBounds(text);
}

int
Font::TextTextureGL(TStringView text, PixelSize &size,
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
