// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageProfile.hpp"
#include "Keys.hpp"
#include "Map.hpp"
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
Load(const ProfileMap &map, PageLayout &_pl, const unsigned page)
{
  char profileKey[32];
  unsigned prefixLen = sprintf(profileKey, "Page%u", page);
  if (prefixLen <= 0)
    return;

  PageLayout pl = PageLayout::Default();
  strcpy(profileKey + prefixLen, "InfoBoxMode");
  if (!map.Get(profileKey, pl.infobox_config.auto_switch))
    return;
  strcpy(profileKey + prefixLen, "InfoBoxPanel");
  if (!map.Get(profileKey, pl.infobox_config.panel))
    return;

  strcpy(profileKey + prefixLen, "Layout");
  unsigned temp = 0;
  map.Get(profileKey, temp);
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
  if (!map.GetEnum(profileKey, pl.bottom) ||
      unsigned(pl.bottom) >= unsigned(PageLayout::Bottom::MAX))
    pl.bottom = PageLayout::Bottom::NOTHING;

  strcpy(profileKey + prefixLen, "Main");
  if (!map.GetEnum(profileKey, pl.main) ||
      unsigned(pl.main) >= unsigned(PageLayout::Main::MAX))
    pl.main = PageLayout::Main::MAP;

  _pl = pl;
}

void
Profile::Load(const ProfileMap &map, PageSettings &settings)
{
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i)
    ::Load(map, settings.pages[i], i);

  settings.Compress();

  map.Get(ProfileKeys::PagesDistinctZoom, settings.distinct_zoom);
}

void
Profile::Save(ProfileMap &map, const PageLayout &page, const unsigned i)
{
  char profileKey[32];
  unsigned prefixLen = sprintf(profileKey, "Page%u", i);
  if (prefixLen <= 0)
    return;
  strcpy(profileKey + prefixLen, "InfoBoxMode");
  map.Set(profileKey, page.infobox_config.auto_switch);
  strcpy(profileKey + prefixLen, "InfoBoxPanel");
  map.Set(profileKey, page.infobox_config.panel);

  strcpy(profileKey + prefixLen, "Layout");
  map.Set(profileKey,
          page.valid
          ? (page.infobox_config.enabled
             ? tlMapAndInfoBoxes
             : tlMap)
          : tlEmpty);

  strcpy(profileKey + prefixLen, "Bottom");
  map.Set(profileKey, (unsigned)page.bottom);

  strcpy(profileKey + prefixLen, "Main");
  map.Set(profileKey, (unsigned)page.main);
}


void
Profile::Save(ProfileMap &map, const PageSettings &settings)
{
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i)
    Save(map, settings.pages[i], i);
}
