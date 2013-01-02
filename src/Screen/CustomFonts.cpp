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

bool
Fonts::LoadCustom()
{
  LoadCustomFont(&infobox, ProfileKeys::FontInfoWindowFont);
  LoadCustomFont(&infobox_small, ProfileKeys::FontTitleSmallWindowFont);
  LoadCustomFont(&title, ProfileKeys::FontTitleWindowFont);
  LoadCustomFont(&cdi, ProfileKeys::FontCDIWindowFont);
  LoadCustomFont(&map_label, ProfileKeys::FontMapLabelFont);
  LoadCustomFont(&map_label_important, ProfileKeys::FontMapLabelImportantFont);
  LoadCustomFont(&map, ProfileKeys::FontMapWindowFont);
  LoadCustomFont(&map_bold, ProfileKeys::FontMapWindowBoldFont);

  return title.IsDefined() && cdi.IsDefined() &&
    map_label.IsDefined() && map_label_important.IsDefined() &&
    map.IsDefined() && map_bold.IsDefined();
}
