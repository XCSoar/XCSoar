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

#include "Pages.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

namespace Pages
{
  unsigned Current = 0;
  static PageSettings settings;
  const unsigned MAX_VALID_LAYOUTS =
    1 + // Nothing
    1 + // Map & Auto InfoBoxes
    1 + // Map (Full Screen)
    InfoBoxSettings::MAX_PANELS;
  unsigned validLayoutsCnt = 0;
  PageSettings::PageLayout validLayouts[MAX_VALID_LAYOUTS];

  void addValidLayout(const PageSettings::PageLayout& pl);
}


void
Pages::Update()
{
  if (settings.pages[Current].topLayout == PageSettings::PageLayout::tlEmpty)
    Current = NextIndex();

  OpenLayout(settings.pages[Current]);
}


unsigned
Pages::NextIndex()
{
  unsigned i = Current;
  do {
    i = (i + 1) % PageSettings::MAX_PAGES;
  } while (settings.pages[i].topLayout == PageSettings::PageLayout::tlEmpty);
  return i;
}


void
Pages::Next()
{
  Current = NextIndex();
  Update();
}


unsigned
Pages::PrevIndex()
{
  unsigned i = Current;
  do {
    i = (i == 0) ? PageSettings::MAX_PAGES - 1 : i - 1;
  } while (settings.pages[i].topLayout == PageSettings::PageLayout::tlEmpty);
  return i;
}


void
Pages::Prev()
{
  Current = PrevIndex();
  Update();
}

void
Pages::Open(unsigned page)
{
  if (page >= PageSettings::MAX_PAGES)
    return;

  if (settings.pages[page].topLayout == PageSettings::PageLayout::tlEmpty)
    return;

  Current = page;
  Update();
}


void
Pages::OpenLayout(const PageSettings::PageLayout &layout)
{
  UIState &ui_state = CommonInterface::SetUIState();

  switch (layout.topLayout) {
  case PageSettings::PageLayout::tlMap:
    XCSoarInterface::main_window.SetFullScreen(true);
    ui_state.auxiliary_enabled = false;
    break;

  case PageSettings::PageLayout::tlMapAndInfoBoxes:
    if (!layout.infoBoxConfig.autoSwitch &&
        layout.infoBoxConfig.panel < InfoBoxSettings::MAX_PANELS) {
      XCSoarInterface::main_window.SetFullScreen(false);
      ui_state.auxiliary_enabled = true;
      ui_state.auxiliary_index = layout.infoBoxConfig.panel;
    }
    else {
      XCSoarInterface::main_window.SetFullScreen(false);
      ui_state.auxiliary_enabled = false;
    }
    break;

  case PageSettings::PageLayout::tlEmpty:
    return;
  }

  InfoBoxManager::SetDirty();
  XCSoarInterface::SendSettingsMap(true);
}


void
Pages::SetLayout(unsigned page, const PageSettings::PageLayout &layout)
{
  if (page >= PageSettings::MAX_PAGES)
    return;

  if (settings.pages[page] != layout) {
    settings.pages[page] = layout;
  }

  if (page == Current)
    Update();
}


void
Pages::Initialise(const PageSettings &_settings)
{
  settings = _settings;
  LoadDefault();
  Update();
}


void
Pages::addValidLayout(const PageSettings::PageLayout &pl)
{
  assert(validLayoutsCnt < MAX_VALID_LAYOUTS);
  validLayouts[validLayoutsCnt++] = pl;
}


void
Pages::LoadDefault()
{
  addValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlEmpty,
                                          PageSettings::InfoBoxConfig(true, 0)));
  addValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlMapAndInfoBoxes,
                                          PageSettings::InfoBoxConfig(true, 0)));
  addValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlMap,
                                          PageSettings::InfoBoxConfig(true, 0)));
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++)
    addValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlMapAndInfoBoxes,
                                            PageSettings::InfoBoxConfig(false, i)));
}


const PageSettings::PageLayout*
Pages::PossiblePageLayout(unsigned i) {
  return (i < validLayoutsCnt) ? &validLayouts[i] : NULL;
}

