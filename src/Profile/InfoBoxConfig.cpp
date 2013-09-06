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

#include "Profile/InfoBoxConfig.hpp"
#include "Profile/Profile.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"

using namespace InfoBoxFactory;

static void
GetV60InfoBoxManagerConfig(InfoBoxSettings &settings)
{
  char profileKey[16];

  assert(settings.MAX_PANELS >= 4);
  strcpy(profileKey, "Info");

  for (unsigned i = 0; i < InfoBoxSettings::Panel::MAX_CONTENTS; ++i) {
    sprintf(profileKey + 4, "%u", i);
    unsigned int temp = 0;
    if (Profile::Get(profileKey, temp)) {
      settings.panels[0].contents[i] = (Type)( temp       & 0xFF);
      settings.panels[1].contents[i] = (Type)((temp >> 8) & 0xFF);
      settings.panels[2].contents[i] = (Type)((temp >> 16) & 0xFF);
      settings.panels[3].contents[i] = (Type)((temp >> 24) & 0xFF);
    }
  }
}

static bool
GetIBType(const char *key, InfoBoxFactory::Type &val)
{
  unsigned _val = val;
  bool ret = Profile::Get(key, _val);

  if (_val >= e_NUM_TYPES)
    return false;

  val = (InfoBoxFactory::Type)_val;
  return ret;
}

void
Profile::Load(InfoBoxSettings &settings)
{
  if (!Get(ProfileKeys::UseFinalGlideDisplayMode, settings.use_final_glide))
    /* default value is "false" for new users, and "true" for existing
       users (to preserve old behaviour and avoid surprises); this is
       a hack to check if this is a new user */
    settings.use_final_glide = Exists(ProfileKeys::InfoBoxGeometry);

  GetEnum(ProfileKeys::InfoBoxGeometry, settings.geometry);

  /* migrate from XCSoar older than 6.7 */
  switch (settings.geometry) {
  case InfoBoxSettings::Geometry::SPLIT_8:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_8:
  case InfoBoxSettings::Geometry::TOP_LEFT_8:
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_SPLIT_8:
    settings.geometry = InfoBoxSettings::Geometry::SPLIT_8;
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_8:
    settings.geometry = InfoBoxSettings::Geometry::TOP_LEFT_8;
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_8:
    settings.geometry = InfoBoxSettings::Geometry::BOTTOM_RIGHT_8;
    break;

  case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
  case InfoBoxSettings::Geometry::RIGHT_5:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_12:
  case InfoBoxSettings::Geometry::RIGHT_24:
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_12:
    settings.geometry = InfoBoxSettings::Geometry::BOTTOM_RIGHT_12;
    break;

  case InfoBoxSettings::Geometry::TOP_LEFT_12:
  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
  case InfoBoxSettings::Geometry::TOP_LEFT_4:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_4:
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_4:
    settings.geometry = InfoBoxSettings::Geometry::TOP_LEFT_4;
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_4:
    settings.geometry = InfoBoxSettings::Geometry::BOTTOM_RIGHT_4;
    break;

  case InfoBoxSettings::Geometry::TOP_8_VARIO:
    break;
  }

  Get(ProfileKeys::AppInverseInfoBox, settings.inverse);
  Get(ProfileKeys::AppInfoBoxColors, settings.use_colors);

  GetEnum(ProfileKeys::AppInfoBoxBorder, settings.border_style);

  GetV60InfoBoxManagerConfig(settings);
  char profileKey[32];
  for (unsigned i = 0; i < settings.MAX_PANELS; ++i) {
    InfoBoxSettings::Panel &panel = settings.panels[i];

    if (i >= settings.PREASSIGNED_PANELS) {
      sprintf(profileKey, "InfoBoxPanel%uName", i);
      Get(profileKey, panel.name.buffer(), panel.name.MAX_SIZE);
      if (panel.name.empty())
        _stprintf(panel.name.buffer(), _T("AUX-%u"), i-2);
    }

    for (unsigned j = 0; j < panel.MAX_CONTENTS; ++j) {
      sprintf(profileKey, "InfoBoxPanel%uBox%u", i, j);
      GetIBType(profileKey, panel.contents[j]);
    }
  }
}

void
Profile::Save(const InfoBoxSettings::Panel &panel, unsigned index)
{
  char profileKey[32];

  if (index >= InfoBoxSettings::PREASSIGNED_PANELS) {
    sprintf(profileKey, "InfoBoxPanel%uName", index);
    Set(profileKey, panel.name);
  }

  for (unsigned j = 0; j < panel.MAX_CONTENTS; ++j) {
    sprintf(profileKey, "InfoBoxPanel%uBox%u", index, j);
    Set(profileKey, panel.contents[j]);
  }
}
