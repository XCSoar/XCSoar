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
  static unsigned current_page = 0;

  static PageSettings settings;

  static constexpr unsigned MAX_VALID_LAYOUTS =
    1 + // Nothing
    1 + // Map & Auto InfoBoxes
    1 + // Map (Full Screen)
    InfoBoxSettings::MAX_PANELS;

  static unsigned num_valid_layouts = 0;
  static PageSettings::PageLayout valid_layouts[MAX_VALID_LAYOUTS];

  static void AddValidLayout(const PageSettings::PageLayout& pl);
}


void
Pages::Update()
{
  if (settings.pages[current_page].top_layout == PageSettings::PageLayout::tlEmpty)
    current_page = NextIndex();

  OpenLayout(settings.pages[current_page]);
}


unsigned
Pages::NextIndex()
{
  unsigned i = current_page;
  do {
    i = (i + 1) % PageSettings::MAX_PAGES;
  } while (settings.pages[i].top_layout == PageSettings::PageLayout::tlEmpty);
  return i;
}


void
Pages::Next()
{
  current_page = NextIndex();
  Update();
}


unsigned
Pages::PrevIndex()
{
  unsigned i = current_page;
  do {
    i = (i == 0) ? PageSettings::MAX_PAGES - 1 : i - 1;
  } while (settings.pages[i].top_layout == PageSettings::PageLayout::tlEmpty);
  return i;
}


void
Pages::Prev()
{
  current_page = PrevIndex();
  Update();
}

void
Pages::Open(unsigned page)
{
  if (page >= PageSettings::MAX_PAGES)
    return;

  if (settings.pages[page].top_layout == PageSettings::PageLayout::tlEmpty)
    return;

  current_page = page;
  Update();
}


void
Pages::OpenLayout(const PageSettings::PageLayout &layout)
{
  UIState &ui_state = CommonInterface::SetUIState();

  switch (layout.top_layout) {
  case PageSettings::PageLayout::tlMap:
    CommonInterface::main_window->SetFullScreen(true);
    ui_state.auxiliary_enabled = false;
    break;

  case PageSettings::PageLayout::tlMapAndInfoBoxes:
    if (!layout.infobox_config.auto_switch &&
        layout.infobox_config.panel < InfoBoxSettings::MAX_PANELS) {
      CommonInterface::main_window->SetFullScreen(false);
      ui_state.auxiliary_enabled = true;
      ui_state.auxiliary_index = layout.infobox_config.panel;
    }
    else {
      CommonInterface::main_window->SetFullScreen(false);
      ui_state.auxiliary_enabled = false;
    }
    break;

  case PageSettings::PageLayout::tlEmpty:
    return;
  }

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();
}


void
Pages::SetLayout(unsigned page, const PageSettings::PageLayout &layout)
{
  if (page >= PageSettings::MAX_PAGES)
    return;

  if (settings.pages[page] != layout) {
    settings.pages[page] = layout;
  }

  if (page == current_page)
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
Pages::AddValidLayout(const PageSettings::PageLayout &pl)
{
  assert(num_valid_layouts < MAX_VALID_LAYOUTS);
  valid_layouts[num_valid_layouts++] = pl;
}


void
Pages::LoadDefault()
{
  AddValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlEmpty,
                                          PageSettings::InfoBoxConfig(true, 0)));
  AddValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlMapAndInfoBoxes,
                                          PageSettings::InfoBoxConfig(true, 0)));
  AddValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlMap,
                                          PageSettings::InfoBoxConfig(true, 0)));
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++)
    AddValidLayout(PageSettings::PageLayout(PageSettings::PageLayout::tlMapAndInfoBoxes,
                                            PageSettings::InfoBoxConfig(false, i)));
}


const PageSettings::PageLayout*
Pages::PossiblePageLayout(unsigned i) {
  return (i < num_valid_layouts) ? &valid_layouts[i] : NULL;
}

