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
#include "OS/FileUtil.hpp"
#include "Compiler.h"

#include <assert.h>

static const char *const all_font_paths[] = {
#ifdef __APPLE__
  "/Library/Fonts/Tahoma.ttf",
  "/Library/Fonts/Arial Narrow.ttf",
#else
  "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed.ttf",
  "/usr/share/fonts/truetype/ttf-droid/DroidSans.ttf",
  "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf",
  "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
  "/usr/share/fonts/truetype/unifont/unifont.ttf",
  "/usr/share/fonts/local/tahoma.ttf",
#endif
  NULL
};

static const char *font_path;

gcc_const
static const char *
DetectFont()
{
  for (const char *const* i = all_font_paths; *i != NULL; ++i)
    if (File::Exists(*i))
      return *i;

  return NULL;
}

void
Font::Initialise()
{
  font_path = DetectFont();
}

bool
Font::_set(const char *file, int ptsize, bool bold, bool italic)
{
  assert(IsScreenInitialized());

  reset();

  font = TTF_OpenFont(file, ptsize);
  if (font == NULL)
    return false;

  int style = TTF_STYLE_NORMAL;
  if (bold)
    style |= TTF_STYLE_BOLD;
  if (italic)
    style |= TTF_STYLE_ITALIC;
  TTF_SetFontStyle(font, style);

  calculate_heights();

  return true;
}

bool
Font::set(const TCHAR *facename, int height, bool bold, bool italic)
{
  LOGFONT lf;
  lf.lfWeight = bold ? 700 : 500;
  lf.lfHeight = height;
  lf.lfItalic = italic;
  return set(lf);
}

bool
Font::set(const LOGFONT &log_font)
{
  assert(IsScreenInitialized());

  if (font_path == NULL)
    return false;

  return _set(font_path, log_font.lfHeight > 0 ? log_font.lfHeight : 10,
              log_font.lfWeight >= 700,
              log_font.lfItalic);
}

void
Font::calculate_heights()
{
  height = TTF_FontHeight(font);
  ascent_height = TTF_FontAscent(font);

  int miny, maxy;
  TTF_GlyphMetrics(font, 'M', NULL, NULL, &miny, &maxy, NULL);

  capital_height = maxy - miny + 1;
}

void
Font::reset()
{
  assert(!defined() || IsScreenInitialized());

  if (font != NULL) {
    assert(IsScreenInitialized());

    TTF_CloseFont(font);
    font = NULL;
  }
}
