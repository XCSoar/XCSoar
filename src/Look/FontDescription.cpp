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
FontDescription::Init(const TCHAR *face,
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
  logfont.lfQuality = ANTIALIASED_QUALITY;

  logfont.lfPitchAndFamily = (monospace ? FIXED_PITCH : VARIABLE_PITCH)
    | FF_DONTCARE;

  _tcscpy(logfont.lfFaceName, face);
}

#endif
