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

#include "Screen/Fonts.hpp"
#include "Screen/Font.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/FontConfig.hpp"

static void
LoadCustomFont(Font *theFont, const TCHAR FontRegKey[])
{
  LOGFONT logfont;
  memset((char *)&logfont, 0, sizeof(LOGFONT));
  if (Profile::GetFont(FontRegKey, &logfont))
    theFont->Set(logfont);
}

void
Fonts::LoadCustom()
{
  LoadCustomFont(&InfoBox, szProfileFontInfoWindowFont);
  LoadCustomFont(&InfoBoxSmall, szProfileFontTitleSmallWindowFont);
  LoadCustomFont(&Title, szProfileFontTitleWindowFont);
  LoadCustomFont(&CDI, szProfileFontCDIWindowFont);
  LoadCustomFont(&MapLabel, szProfileFontMapLabelFont);
  LoadCustomFont(&MapLabelImportant, szProfileFontMapLabelImportantFont);
  LoadCustomFont(&Map, szProfileFontMapWindowFont);
  LoadCustomFont(&MapBold, szProfileFontMapWindowBoldFont);
}
