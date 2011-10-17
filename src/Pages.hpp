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

#ifndef XCSOAR_PAGES_HPP
#define XCSOAR_PAGES_HPP

#include <tchar.h>

namespace Pages
{
  const unsigned MAX_PAGES = 8;

  struct InfoBoxConfig {
    bool autoSwitch;
    unsigned panel;

    InfoBoxConfig(bool autoSwitch = true, unsigned panel = 0)
      : autoSwitch(autoSwitch), panel(panel) {}

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

    PageLayout(eTopLayout topLayout=tlEmpty, InfoBoxConfig infoBoxConfig=InfoBoxConfig()) :
      topLayout(topLayout), infoBoxConfig(infoBoxConfig) {}

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

  /**
   * Opens the given page.
   * @param page The page to open
   */
  void Open(unsigned page);
  /**
   * Opens the next page.
   */
  void Next();
  /**
   * Opens the previous page.
   */
  void Prev();

  /**
   * Retrieve current layout
   */
  const PageLayout& get_current();

  /**
   * Opens the given layout.
   * Attention! Internally the previous page is still selected.
   * @param layout The layout to open
   */
  void OpenLayout(const PageLayout &layout);

  /**
   * Assigns a new layout to the given page
   * @param page The page to change
   * @param layout The layout that should be assigned
   */
  void SetLayout(unsigned page, const PageLayout &layout);
  /**
   * Returns the layout of the given page
   * @param page The page to look for
   * @return The layout of the given page
   */
  PageLayout* GetLayout(unsigned page);

  void SavePageToProfile(unsigned page);
  void SaveToProfile();
  void LoadPageFromProfile(unsigned page);
  void LoadFromProfile();
  void LoadDefault();
  const PageLayout* PossiblePageLayout(unsigned i);

  unsigned NextIndex();
  unsigned PrevIndex();

  void Update();
};

#endif
