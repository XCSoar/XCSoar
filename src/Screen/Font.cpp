/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifdef ENABLE_SDL

bool
Font::set(const char *file, int ptsize, bool bold, bool italic)
{
  reset();

  if (!TTF_WasInit())
    TTF_Init();

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
Font::set(const LOGFONT &log_font)
{
  // XXX hard coded path
  const char *dejavu_ttf =
    "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed.ttf";

  return set(dejavu_ttf, log_font.lfHeight > 0 ? log_font.lfHeight : 10,
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
  if (font != NULL) {
    TTF_CloseFont(font);
    font = NULL;
  }
}

#else /* !ENABLE_SDL */

#include "Screen/BufferCanvas.hpp"
#include "Asset.hpp"

bool
Font::set(const TCHAR* facename, int height, bool bold, bool italic)
{
  LOGFONT font;
  memset((char *)&font, 0, sizeof(LOGFONT));

  _tcscpy(font.lfFaceName, facename);
  font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  font.lfHeight = (long)height;
  font.lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  font.lfItalic = italic;
  font.lfQuality = ANTIALIASED_QUALITY;
  return Font::set(font);
}

bool
Font::set(const LOGFONT &log_font)
{
  reset();

  font = ::CreateFontIndirect(&log_font);
  if (font == NULL)
    return false;

  if (GetObjectType(font) != OBJ_FONT) {
    reset();
    return false;
  }

  calculate_heights();

  return true;
}

void
Font::calculate_heights()
{
  VirtualCanvas canvas(1, 1);
  canvas.select(*this);

  TEXTMETRIC tm;
  ::GetTextMetrics(canvas, &tm);

  height = tm.tmHeight;
  ascent_height = tm.tmAscent;

  if (is_altair()) {
    // JMW: don't know why we need this in GNAV, but we do.

    BufferCanvas buffer(canvas, tm.tmAveCharWidth, tm.tmHeight);
    const HWColor white = buffer.map(Color::WHITE);

    buffer.background_opaque();
    buffer.set_background_color(Color::WHITE);
    buffer.set_text_color(Color::BLACK);
    buffer.select(*this);

    RECT rec;
    rec.left = 0;
    rec.top = 0;
    rec.right = tm.tmAveCharWidth;
    rec.bottom = tm.tmHeight;
    buffer.text_opaque(0, 0, rec, _T("M"));

    int top = tm.tmHeight, bottom = 0;

    for (int x = 0; x < tm.tmAveCharWidth; ++x) {
      for (int y = 0; y < tm.tmHeight; ++y) {
        if (buffer.get_pixel(x, y) != white) {
          if (top > y)
            top = y;
          if (bottom < y)
            bottom = y;
        }
      }
    }

    capital_height = bottom - top + 1;
  } else {
    // This works for PPC
    capital_height = tm.tmAscent - 1 - tm.tmHeight / 10;
  }
}

void
Font::reset()
{
  if (font != NULL) {
    ::DeleteObject(font);
    font = NULL;
  }
}

#endif /* !ENABLE_SDL */
