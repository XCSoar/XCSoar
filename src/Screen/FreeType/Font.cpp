/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Custom/Files.hpp"
#include "Init.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>

#include <assert.h>

static const char *font_path;
static const char *bold_font_path;
static const char *monospace_font_path;

gcc_const
static inline FT_Long
FT_FLOOR(FT_Long x)
{
  return (x & -64) / 64;
}

gcc_const
static inline FT_Long
FT_CEIL(FT_Long x)
{
  return ((x + 63) & -64) / 64;
}

void
Font::Initialise()
{
  font_path = FindDefaultFont();
  bold_font_path = FindDefaultBoldFont();
  monospace_font_path = FindDefaultMonospaceFont();
}

gcc_pure
static unsigned
GetCapitalHeight(FT_Face face)
{
  FT_UInt i = FT_Get_Char_Index(face, 'M');
  if (i == 0)
    return 0;

  FT_Error error = FT_Load_Glyph(face, i, FT_LOAD_DEFAULT);
  if (error)
    return 0;

  return FT_CEIL(face->glyph->metrics.height);
}

bool
Font::LoadFile(const char *file, UPixelScalar ptsize, bool bold, bool italic)
{
  assert(IsScreenInitialized());

  Destroy();

  FT_Face new_face = FreeType::Load(file);
  if (new_face == nullptr)
    return false;

  FT_Error error = ::FT_Set_Pixel_Sizes(new_face, 0, ptsize);
  if (error) {
    ::FT_Done_Face(new_face);
    return false;
  }

  const FT_Fixed y_scale = new_face->size->metrics.y_scale;

  height = FT_CEIL(FT_MulFix(new_face->height, y_scale));
  ascent_height = FT_CEIL(FT_MulFix(new_face->ascender, y_scale));

  capital_height = ::GetCapitalHeight(new_face);
  if (capital_height == 0)
    capital_height = height;

  // TODO: handle bold/italic

  face = new_face;
  return true;
}

bool
Font::Load(const TCHAR *facename, UPixelScalar height, bool bold, bool italic)
{
  LOGFONT lf;
  lf.lfWeight = bold ? 700 : 500;
  lf.lfHeight = height;
  lf.lfItalic = italic;
  lf.lfPitchAndFamily = FF_DONTCARE | VARIABLE_PITCH;
  return Load(lf);
}

bool
Font::Load(const LOGFONT &log_font)
{
  assert(IsScreenInitialized());

  bool bold = log_font.lfWeight >= 700;

  const char *path;
  if ((log_font.lfPitchAndFamily & 0x03) == FIXED_PITCH &&
      monospace_font_path != nullptr) {
    path = monospace_font_path;
  } else if (bold && bold_font_path != nullptr) {
    /* a bold variant of the font exists: clear the "bold" flag, so
       SDL_TTF does not apply it again */
    path = bold_font_path;
    bold = false;
  } else {
    path = font_path;
  }

  if (path == NULL)
    return false;

  return LoadFile(path, log_font.lfHeight > 0 ? log_font.lfHeight : 10,
                  bold, log_font.lfItalic);
}

void
Font::Destroy()
{
  if (!IsDefined())
    return;

  assert(IsScreenInitialized());

  ::FT_Done_Face(face);
  face = nullptr;
}

PixelSize
Font::TextSize(const TCHAR *text) const
{
  // TODO: parse UTF-8
  // TODO: kerning
  // TODO: overhang

  const FT_Face face = this->face;

  int x = 0, minx = 0, maxx = 0;

  for (const TCHAR *p = text; *p != 0; ++p) {
    const auto ch = *p;

    FT_UInt i = FT_Get_Char_Index(face, ch);
    if (i == 0)
      continue;

    FT_Error error = FT_Load_Glyph(face, i, FT_LOAD_DEFAULT);
    if (error)
      continue;

    const FT_GlyphSlot glyph = face->glyph;
    const FT_Glyph_Metrics &metrics = glyph->metrics;

    const int glyph_minx = FT_FLOOR(metrics.horiBearingX);
    const int glyph_maxx = minx + FT_CEIL(metrics.width);
    const int glyph_advance = FT_CEIL(metrics.horiAdvance);

    int z = x + glyph_minx;
    if (z < minx)
      minx = z;

    z = x + std::max(glyph_maxx, glyph_advance);
    if (z > maxx)
      maxx = z;

    x += glyph_advance;
  }

  return PixelSize{unsigned(maxx - minx), height};
}

static void
RenderGlyph(uint8_t *buffer, unsigned buffer_width, unsigned buffer_height,
            const FT_Bitmap &bitmap, int x, int y)
{
  const uint8_t *src = (const uint8_t *)bitmap.buffer;
  int width = bitmap.width, height = bitmap.rows;
  int pitch = bitmap.pitch;

  if (x < 0) {
    src -= x;
    width += x;
    x = 0;
  }

  if (unsigned(x) >= buffer_width || width <= 0)
    return;

  if (unsigned(x + width) > buffer_width)
    width = buffer_width - x;

  if (y < 0) {
    src -= y * pitch;
    height += y;
    y = 0;
  }

  if (unsigned(y) >= buffer_height || height <= 0)
    return;

  if (unsigned(y + height) > buffer_height)
    height = buffer_height - y;

  buffer += unsigned(y) * buffer_width + unsigned(x);
  for (const uint8_t *end = src + height * pitch;
       src != end; src += pitch, buffer += buffer_width)
    // TODO: mix with previous character?
    std::copy(src, src + width, buffer);
}

static void
RenderGlyph(uint8_t *buffer, size_t width, size_t height,
            FT_GlyphSlot glyph, int x, int y)
{
  FT_Error error = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
  if (error)
    return;

  RenderGlyph(buffer, width, height, glyph->bitmap, x, y);
}

void
Font::Render(const TCHAR *text, const PixelSize size, void *_buffer) const
{
  uint8_t *buffer = (uint8_t *)_buffer;
  std::fill(buffer, buffer + BufferSize(size), 0);

  const FT_Face face = this->face;

  int x = 0, minx = 0;

  for (const TCHAR *p = text; *p != 0; ++p) {
    const auto ch = *p;

    FT_UInt i = FT_Get_Char_Index(face, ch);
    if (i == 0)
      continue;

    FT_Error error = FT_Load_Glyph(face, i, FT_LOAD_DEFAULT);
    if (error)
      continue;

    const FT_GlyphSlot glyph = face->glyph;
    const FT_Glyph_Metrics &metrics = glyph->metrics;

    const int glyph_minx = FT_FLOOR(metrics.horiBearingX);
    const int glyph_advance = FT_CEIL(metrics.horiAdvance);

    int z = x + glyph_minx;
    if (z < minx)
      minx = z;

    const int glyph_maxy = FT_FLOOR(metrics.horiBearingY);

    RenderGlyph((uint8_t *)buffer, size.cx, size.cy,
                glyph, x - minx, ascent_height - glyph_maxy);

    x += glyph_advance;
  }
}
