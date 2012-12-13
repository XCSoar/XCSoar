/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include <type_traits>

#include <tchar.h>

struct InfoBoxSettings;

struct PageSettings {
  static constexpr unsigned MAX_PAGES = 8;

  struct InfoBoxConfig {
    bool auto_switch;
    unsigned panel;

    InfoBoxConfig() = default;

    constexpr InfoBoxConfig(bool _auto_switch, unsigned _panel)
      : auto_switch(_auto_switch), panel(_panel) {}

    void SetDefaults() {
      auto_switch = true;
      panel = 0;
    }

    bool operator==(const InfoBoxConfig &other) const {
      return auto_switch == other.auto_switch &&
             panel == other.panel;
    }

    bool operator!=(const InfoBoxConfig &other) const {
      return !(*this == other);
    }
  };

  struct PageLayout
  {
    enum eTopLayout {
      tlEmpty,
      tlMap,
      tlMapAndInfoBoxes,
      tlLAST = tlMapAndInfoBoxes
    } top_layout;

    InfoBoxConfig infobox_config;

    PageLayout() = default;

    constexpr PageLayout(eTopLayout _top_layout, InfoBoxConfig _infobox_config)
      :top_layout(_top_layout), infobox_config(_infobox_config) {}

    void SetDefaults() {
      top_layout = tlEmpty;
      infobox_config.SetDefaults();
    }

    void MakeTitle(const InfoBoxSettings &info_box_settings,
                   TCHAR *str, const bool concise=false) const;

    bool operator==(const PageLayout &other) const {
      return top_layout == other.top_layout &&
             infobox_config == other.infobox_config;
    }

    bool operator!=(const PageLayout &other) const {
      return !(*this == other);
    }
  };

  PageLayout pages[MAX_PAGES];

  void SetDefaults();
};

static_assert(std::is_trivial<PageSettings>::value, "type is not trivial");

#endif
