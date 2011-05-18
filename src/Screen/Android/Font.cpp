/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Java/Global.hpp"
#include "Java/Class.hpp"
#include "Java/String.hpp"
#include "Android/TextUtil.hpp"

#include <assert.h>

bool
Font::set(const LOGFONT &log)
{
  return set(log.lfFaceName, (int) log.lfHeight,
             log.lfWeight > 600, log.lfItalic != 0);
}

/*
 * create a new instance of org.xcsoar.TextUtil and store it with a global
 * reference in textUtilObject member.
 */
bool
Font::set(const TCHAR *facename, int height, bool bold, bool italic)
{
  assert(IsScreenInitialized());

  delete textUtilObject;
  textUtilObject = TextUtil::create(facename, height, bold, italic);
  if (!textUtilObject)
    return false;

  this->height = textUtilObject->get_height();
  style = textUtilObject->get_style();
  ascent_height = textUtilObject->get_ascent_height();
  capital_height = textUtilObject->get_capital_height();
  line_spacing = textUtilObject->get_line_spacing();

  return textUtilObject != NULL;
}

void
Font::reset()
{
  assert(!defined() || IsScreenInitialized());

  delete textUtilObject;
  textUtilObject = NULL;
}

void
Font::text_width(const TCHAR *text, PixelSize &result) const
{

  if (!textUtilObject)
    return;

  std::pair<unsigned, unsigned> size = textUtilObject->getTextBounds(text);
  result.cx = size.first;
  result.cy = size.second;
}

int
Font::text_texture_gl(const TCHAR *text, PixelSize &size,
                      const Color &fg, const Color &bg) const
{
  if (!textUtilObject)
    return NULL;

  size.cx = size.cy = 0;
  text_width(text, size);
  if (size.cx == 0 || size.cy == 0)
    return NULL;

  return textUtilObject->getTextTextureGL(text,
                                          fg.red(), fg.green(), fg.blue(),
                                          bg.red(), bg.green(), bg.blue());
}
