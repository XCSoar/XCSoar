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

#ifndef XCSOAR_PAGE_SETTINGS_HPP
#define XCSOAR_PAGE_SETTINGS_HPP

#include "Util/TypeTraits.hpp"
#include "Compiler.h"

#include <tchar.h>

struct PageSettings {
  static gcc_constexpr_data unsigned MAX_PAGES = 8;

  struct InfoBoxConfig {
    bool autoSwitch;
    unsigned panel;

    InfoBoxConfig() = default;

    InfoBoxConfig(bool autoSwitch, unsigned panel)
      : autoSwitch(autoSwitch), panel(panel) {}

    void SetDefaults() {
      autoSwitch = true;
      panel = 0;
    }

    bool operator==(const InfoBoxConfig& ibc) const {
      if (autoSwitch != ibc.autoSwitch)
        return false;
      if (panel != ibc.panel)
        return false;
      return true;
    }

    bool operator!=(const InfoBoxConfig& ibc) const {
      return !(*this == ibc);
    }
  };

  struct PageLayout
  {
    enum eTopLayout {
      tlEmpty,
      tlMap,
      tlMapAndInfoBoxes,
      tlLAST = tlMapAndInfoBoxes
    } topLayout;

    InfoBoxConfig infoBoxConfig;

    PageLayout() = default;

    PageLayout(eTopLayout topLayout, InfoBoxConfig infoBoxConfig) :
      topLayout(topLayout), infoBoxConfig(infoBoxConfig) {}

    void SetDefaults() {
      topLayout = tlEmpty;
      infoBoxConfig.SetDefaults();
    }

    void MakeTitle(TCHAR* str, const bool concise=false) const;

    bool operator==(const PageLayout& pl) const {
      if (topLayout != pl.topLayout)
        return false;
      if (infoBoxConfig != pl.infoBoxConfig)
        return false;
      return true;
    }

    bool operator!=(const PageLayout& pl) const {
      return !(*this == pl);
    }
  };

  PageLayout pages[MAX_PAGES];

  void SetDefaults();
};

static_assert(is_trivial<PageSettings>::value, "type is not trivial");

#endif
