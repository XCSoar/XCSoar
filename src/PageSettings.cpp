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

#include "PageSettings.hpp"
#include "Language/Language.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

#include <stdio.h>

void
PageSettings::PageLayout::MakeTitle(TCHAR* buffer, const bool concise) const
{
  switch (topLayout) {
  case PageSettings::PageLayout::tlMap:
    if (concise)
      _tcscpy(buffer, _("Info Hide"));
    else
      _tcscpy(buffer, _("Map (Full screen)"));
    break;

  case PageSettings::PageLayout::tlMapAndInfoBoxes:
    if (!infoBoxConfig.autoSwitch &&
        infoBoxConfig.panel < InfoBoxSettings::MAX_PANELS) {
      if (concise) {
        _tcscpy(buffer, _("Info "));
        _tcscat(buffer, InfoBoxManager::GetPanelName(infoBoxConfig.panel));
      } else {
        _tcscpy(buffer, _("Map and InfoBoxes "));
        _tcscat(buffer, InfoBoxManager::GetPanelName(infoBoxConfig.panel));
      }
    }
    else {
      if (concise) {
        _tcscpy(buffer, _("Info Auto"));
      } else {
        _tcscpy(buffer, _("Map and InfoBoxes (Auto)"));
      }
    }
    break;

  default:
    _tcscpy(buffer, _T("---"));
    break;
  }
}

void
PageSettings::SetDefaults()
{
  pages[0] = PageLayout(PageLayout::tlMapAndInfoBoxes, InfoBoxConfig(true, 0));
  pages[1] = PageLayout(PageLayout::tlMap, InfoBoxConfig(true, 0));

  for (unsigned i = 2; i < MAX_PAGES; ++i)
    pages[i].SetDefaults();
}
