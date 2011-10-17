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

#include "Profile/PageProfile.hpp"
#include "Profile/Profile.hpp"
#include "PageSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"

static void
Load(PageSettings::PageLayout &_pl, const unsigned page)
{
  TCHAR profileKey[32];
  unsigned prefixLen = _stprintf(profileKey, _T("Page%u"), page);
  if (prefixLen <= 0)
    return;

  PageSettings::PageLayout pl;
  _tcscpy(profileKey + prefixLen, _T("InfoBoxMode"));
  if (!Profile::Get(profileKey, pl.infoBoxConfig.autoSwitch))
    return;
  _tcscpy(profileKey + prefixLen, _T("InfoBoxPanel"));
  if (!Profile::Get(profileKey, pl.infoBoxConfig.panel))
    return;
  _tcscpy(profileKey + prefixLen, _T("Layout"));
  unsigned temp = 0;
  if (!Profile::Get(profileKey, temp))
    return;
  pl.topLayout = (PageSettings::PageLayout::eTopLayout) temp;
  if (pl.topLayout > PageSettings::PageLayout::tlLAST)
    return;
  if (pl.infoBoxConfig.panel >= InfoBoxSettings::MAX_PANELS)
    return;
  if (page == 0 && pl.topLayout == PageSettings::PageLayout::tlEmpty)
    return;

  _pl = pl;
}

void
Profile::Load(PageSettings &settings)
{
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i)
    ::Load(settings.pages[i], i);
}

void
Profile::Save(const PageSettings::PageLayout &page, const unsigned i)
{
  TCHAR profileKey[32];
  unsigned prefixLen = _stprintf(profileKey, _T("Page%u"), i);
  if (prefixLen <= 0)
    return;
  _tcscpy(profileKey + prefixLen, _T("InfoBoxMode"));
  Profile::Set(profileKey, page.infoBoxConfig.autoSwitch);
  _tcscpy(profileKey + prefixLen, _T("InfoBoxPanel"));
  Profile::Set(profileKey, page.infoBoxConfig.panel);
  _tcscpy(profileKey + prefixLen, _T("Layout"));
  Profile::Set(profileKey, page.topLayout);
}


void
Profile::Save(const PageSettings &settings)
{
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i)
    Save(settings.pages[i], i);
}
