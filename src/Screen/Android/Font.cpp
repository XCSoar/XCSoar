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

#include "Screen/Font.hpp"
#include "Screen/Debug.hpp"
#include "Look/FontDescription.hpp"
#include "Java/Global.hxx"
#include "Java/Class.hxx"
#include "Java/String.hxx"
#include "Android/TextUtil.hpp"
#include "Util/StringCompare.hxx"

#include <assert.h>

/*
 * create a new instance of org.xcsoar.TextUtil and store it with a global
 * reference in text_util_object member.
 */
bool
Font::Load(const FontDescription &d)
{
  assert(IsScreenInitialized());

  delete text_util_object;
  text_util_object = TextUtil::create(d);
  if (!text_util_object)
    return false;

  this->height = text_util_object->get_height();
  ascent_height = text_util_object->get_ascent_height();
  capital_height = text_util_object->get_capital_height();
  line_spacing = text_util_object->GetLineSpacing();

  return true;
}

void
Font::Destroy()
{
  assert(!IsDefined() || IsScreenInitialized());

  delete text_util_object;
  text_util_object = nullptr;
}

PixelSize
Font::TextSize(const TCHAR *text) const
{
  if (text_util_object == nullptr) {
    PixelSize empty = { 0, 0 };
    return empty;
  }

  return text_util_object->getTextBounds(text);
}

int
Font::TextTextureGL(const TCHAR *text, PixelSize &size,
                    PixelSize &allocated_size) const
{
  if (!text_util_object)
    return 0;

  if (StringIsEmpty(text))
    return 0;

  const TextUtil::Texture texture =
    text_util_object->getTextTextureGL(text);
  size.cx = texture.width;
  size.cy = texture.height;
  allocated_size.cx = texture.allocated_width;
  allocated_size.cy = texture.allocated_height;
  return texture.id;
}
