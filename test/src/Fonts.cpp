/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Fonts.hpp"
#include "Screen/Font.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

Font normal_font, small_font, bold_font, monospace_font;

static const TCHAR *
GetStandardMonospaceFontFace()
{
  if (IsAndroid())
    return _T("Droid Sans Mono");

  return _T("Courier");
}

static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, int height,
                  bool bold = false, bool italic = false,
                  bool variable_pitch = true)
{
  memset((char *)font, 0, sizeof(LOGFONT));

  _tcscpy(font->lfFaceName, facename);

#ifdef WIN32
  font->lfPitchAndFamily = (variable_pitch ? VARIABLE_PITCH : FIXED_PITCH)
                          | FF_DONTCARE;
#endif

  font->lfHeight = (long)height;
  font->lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  font->lfItalic = italic;

#ifdef WIN32
  if (IsAltair())
    font->lfQuality = NONANTIALIASED_QUALITY;
  else
    font->lfQuality = ANTIALIASED_QUALITY;
#endif
}

void
InitialiseFonts()
{
  const TCHAR *face = _T("Tahoma");

#ifndef USE_GDI
  UPixelScalar FontHeight = Layout::SmallScale(IsAndroid() ? 30 : 24);
#else
  UPixelScalar FontHeight = Layout::SmallScale(35);
#endif

  LOGFONT lf;
  InitialiseLogfont(&lf, face, FontHeight / 2);
  normal_font.Load(lf);

  InitialiseLogfont(&lf, face, FontHeight / 2 - Layout::Scale(2));
  small_font.Load(lf);

  InitialiseLogfont(&lf, face, FontHeight / 2, true);
  bold_font.Load(lf);

  InitialiseLogfont(&lf, GetStandardMonospaceFontFace(),
                    10, false, false, false);
  monospace_font.Load(lf);
}

void
DeinitialiseFonts()
{
  monospace_font.Destroy();
  bold_font.Destroy();
  small_font.Destroy();
  normal_font.Destroy();
}
