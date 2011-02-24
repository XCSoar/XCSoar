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

#ifndef XCSOAR_PROFILE_INFOBOX_CONFIG_HPP
#define XCSOAR_PROFILE_INFOBOX_CONFIG_HPP

#include <tchar.h>

class InfoBoxPanelConfig {
public:
  InfoBoxPanelConfig();
  bool IsEmpty() const;

  static const unsigned int MAX_INFOBOXES = 12;
  static const unsigned int MAX_PANEL_NAME_LENGTH = 32;
  TCHAR name[MAX_PANEL_NAME_LENGTH];
  unsigned int infoBoxID[MAX_INFOBOXES];
  bool modified;
};

class InfoBoxManagerConfig {
public:
  InfoBoxManagerConfig();

  static const unsigned int MAX_INFOBOX_PANELS = 4;
  static const unsigned int PREASSIGNED_PANELS = 3;
  InfoBoxPanelConfig panel[MAX_INFOBOX_PANELS];
};

namespace Profile
{
  void GetInfoBoxManagerConfig(InfoBoxManagerConfig &config);
  void SetInfoBoxManagerConfig(const InfoBoxManagerConfig &config);
};

#endif
