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

#include "Profile/PageProfile.hpp"
#include "Profile/Profile.hpp"
#include "PageSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"

/**
 * Old enum moved from PageSettings.
 */
enum eTopLayout {
  tlEmpty,
  tlMap,
  tlMapAndInfoBoxes,
};

static void
Load(PageLayout &_pl, const unsigned page)
{
  char profileKey[32];
  unsigned prefixLen = sprintf(profileKey, "Page%u", page);
  if (prefixLen <= 0)
    return;

  PageLayout pl = PageLayout::Default();
  strcpy(profileKey + prefixLen, "InfoBoxMode");
  if (!Profile::Get(profileKey, pl.infobox_config.auto_switch))
    return;
  strcpy(profileKey + prefixLen, "InfoBoxPanel");
  if (!Profile::Get(profileKey, pl.infobox_config.panel))
    return;

  strcpy(profileKey + prefixLen, "Layout");
  unsigned temp = 0;
  Profile::Get(profileKey, temp);
  switch (temp) {
  case tlEmpty:
    pl.valid = false;
    break;

  case tlMap:
    pl.infobox_config.enabled = false;
    break;
  }

  if (pl.infobox_config.panel >= InfoBoxSettings::MAX_PANELS)
    return;
  if (page == 0 && !pl.IsDefined())
    return;

  strcpy(profileKey + prefixLen, "Bottom");
  if (!Profile::GetEnum(profileKey, pl.bottom) ||
      unsigned(pl.bottom) >= unsigned(PageLayout::Bottom::MAX))
    pl.bottom = PageLayout::Bottom::NOTHING;

  strcpy(profileKey + prefixLen, "Main");
  if (!Profile::GetEnum(profileKey, pl.main) ||
      unsigned(pl.main) >= unsigned(PageLayout::Main::MAX))
    pl.main = PageLayout::Main::MAP;

  _pl = pl;
}

void
Profile::Load(PageSettings &settings)
{
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i)
    ::Load(settings.pages[i], i);

  settings.Compress();

  Get(ProfileKeys::PagesDistinctZoom, settings.distinct_zoom);
}

void
Profile::Save(const PageLayout &page, const unsigned i)
{
  char profileKey[32];
  unsigned prefixLen = sprintf(profileKey, "Page%u", i);
  if (prefixLen <= 0)
    return;
  strcpy(profileKey + prefixLen, "InfoBoxMode");
  Profile::Set(profileKey, page.infobox_config.auto_switch);
  strcpy(profileKey + prefixLen, "InfoBoxPanel");
  Profile::Set(profileKey, page.infobox_config.panel);

  strcpy(profileKey + prefixLen, "Layout");
  Profile::Set(profileKey,
               page.valid
               ? (page.infobox_config.enabled
                  ? tlMapAndInfoBoxes
                  : tlMap)
               : tlEmpty);

  strcpy(profileKey + prefixLen, "Bottom");
  Profile::Set(profileKey, (unsigned)page.bottom);

  strcpy(profileKey + prefixLen, "Main");
  Profile::Set(profileKey, (unsigned)page.main);
}


void
Profile::Save(const PageSettings &settings)
{
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i)
    Save(settings.pages[i], i);
}
