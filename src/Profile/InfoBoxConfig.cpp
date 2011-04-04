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

#include "Profile/InfoBoxConfig.hpp"
#include "Profile/Profile.hpp"
#include "Language.hpp"


InfoBoxPanelConfig::InfoBoxPanelConfig() : modified(false)
{
  name[0] = 0;
  for (unsigned int i = 0; i < MAX_INFOBOXES; i++)
    infoBoxID[i] = 0;
}


bool InfoBoxPanelConfig::IsEmpty() const
{
  for (unsigned int i = 0; i < MAX_INFOBOXES; i++)
    if (infoBoxID[i] != 0)
      return false;
  return true;
}


InfoBoxManagerConfig::InfoBoxManagerConfig()
{
  static const unsigned int DFLT_CONFIG_BOXES = 9;
  static const unsigned int DFLT_CONFIG_PANELS = 4;
  static const int dflt_IDs[DFLT_CONFIG_PANELS][DFLT_CONFIG_BOXES] = {
    { 0x0E, 0x0B, 0x16, 0x31, 0x30, 0x21, 0x07, 0x0F, 0x2D },
    { 0x0E, 0x0B, 0x03, 0x2B, 0x30, 0x21, 0x11, 0x0F, 0x2D },
    { 0x0E, 0x12, 0x03, 0x2B, 0x26, 0x21, 0x29, 0x0F, 0x2D },
    { 0x34, 0x33, 0x31, 0x00, 0x06, 0x19, 0x27, 0x25, 0x1A }
  };

  assert(MAX_INFOBOX_PANELS >= DFLT_CONFIG_PANELS);
  assert(InfoBoxPanelConfig::MAX_INFOBOXES >= DFLT_CONFIG_BOXES);

  _tcscpy(panel[0].name, _T("Circling"));
  _tcscpy(panel[1].name, _T("Cruise"));
  _tcscpy(panel[2].name, _T("FinalGlide"));
  for (unsigned int i = PREASSIGNED_PANELS; i < MAX_INFOBOX_PANELS; i++)
    _stprintf(panel[i].name, _T("AUX-%u"), i-2);

  for (unsigned int i = 0; i < DFLT_CONFIG_PANELS; i++)
    for (unsigned int j = 0; j < DFLT_CONFIG_BOXES; j++)
      panel[i].infoBoxID[j] = dflt_IDs[i][j];
}


static void
GetV60InfoBoxManagerConfig(InfoBoxManagerConfig &config) {
  TCHAR profileKey[16];

  assert(InfoBoxManagerConfig::MAX_INFOBOX_PANELS >= 4);
  _tcscpy(profileKey, _T("Info"));

  for (unsigned int i = 0; i < InfoBoxPanelConfig::MAX_INFOBOXES; i++) {
    _stprintf(profileKey+4, _T("%u"), i);
    unsigned int temp = 0;
    if (Profile::Get(profileKey, temp)) {
      config.panel[0].infoBoxID[i] = temp & 0xFF;
      config.panel[1].infoBoxID[i] = (temp >> 8) & 0xFF;
      config.panel[2].infoBoxID[i] = (temp >> 16) & 0xFF;
      config.panel[3].infoBoxID[i] = (temp >> 24) & 0xFF;
    }
  }
}


void
Profile::GetInfoBoxManagerConfig(InfoBoxManagerConfig &config)
{
  GetV60InfoBoxManagerConfig(config);
  TCHAR profileKey[32];
  for (unsigned int i = 0; i < InfoBoxManagerConfig::MAX_INFOBOX_PANELS; i++) {
    if (i >= InfoBoxManagerConfig::PREASSIGNED_PANELS) {
      _stprintf(profileKey, _T("InfoBoxPanel%uName"), i);
      Get(profileKey, config.panel[i].name, InfoBoxPanelConfig::MAX_PANEL_NAME_LENGTH);
      if (config.panel[i].name[0] == 0)
        _stprintf(config.panel[i].name, _T("AUX-%u"), i-2);
    }
    for (unsigned int j = 0; j < InfoBoxPanelConfig::MAX_INFOBOXES; j++) {
      _stprintf(profileKey, _T("InfoBoxPanel%uBox%u"), i, j);
      Get(profileKey, config.panel[i].infoBoxID[j]);
    }
  }
}


void
Profile::SetInfoBoxManagerConfig(const InfoBoxManagerConfig &config)
{
  TCHAR profileKey[32];
  for (unsigned int i = 0; i < InfoBoxManagerConfig::MAX_INFOBOX_PANELS; i++) {
    if (config.panel[i].modified) {
      if (i >= InfoBoxManagerConfig::PREASSIGNED_PANELS) {
        _stprintf(profileKey, _T("InfoBoxPanel%uName"), i);
        Set(profileKey, config.panel[i].name);
      }
      for (unsigned int j = 0; j < InfoBoxPanelConfig::MAX_INFOBOXES; j++) {
        _stprintf(profileKey, _T("InfoBoxPanel%uBox%u"), i, j);
        Set(profileKey, config.panel[i].infoBoxID[j]);
      }
    }
  }
}
