// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FontDescription.hpp"

#ifdef USE_GDI
#include "StandardFonts.hpp"
#include "Asset.hpp"

FontDescription::FontDescription(unsigned height,
                                 bool bold, bool italic,
                                 bool monospace)
{
  Init(monospace ? GetStandardMonospaceFontFace() : GetStandardFontFace(),
       -int(height), bold, italic, monospace);
}

void
FontDescription::Init(const char *face,
                      int height,
                      bool bold, bool italic,
                      bool monospace)
{
  logfont.lfHeight = (long)height;
  logfont.lfWidth = 0;
  logfont.lfEscapement = 0;
  logfont.lfOrientation = 0;
  logfont.lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  logfont.lfItalic = italic;
  logfont.lfUnderline = false;
  logfont.lfStrikeOut = false;
  logfont.lfCharSet = ANSI_CHARSET;
  logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality = CLEARTYPE_QUALITY;

  logfont.lfPitchAndFamily = (monospace ? FIXED_PITCH : VARIABLE_PITCH)
    | FF_DONTCARE;

  strcpy(logfont.lfFaceName, face);
}

#endif
