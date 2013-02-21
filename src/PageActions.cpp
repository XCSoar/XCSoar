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

#include "PageActions.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "MainWindow.hpp"
#include "CrossSection/CrossSectionWidget.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"

void
Pages::Update()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  UIState &ui_state = CommonInterface::SetUIState();

  if (!settings.pages[ui_state.page_index].IsDefined())
    ui_state.page_index = NextIndex();

  OpenLayout(settings.pages[ui_state.page_index]);
}


unsigned
Pages::NextIndex()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const UIState &ui_state = CommonInterface::SetUIState();

  unsigned i = ui_state.page_index;
  do {
    i = (i + 1) % PageSettings::MAX_PAGES;
  } while (!settings.pages[i].IsDefined());
  return i;
}


void
Pages::Next()
{
  UIState &ui_state = CommonInterface::SetUIState();

  ui_state.page_index = NextIndex();
  Update();
}


unsigned
Pages::PrevIndex()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const UIState &ui_state = CommonInterface::SetUIState();

  unsigned i = ui_state.page_index;
  do {
    i = (i == 0) ? PageSettings::MAX_PAGES - 1 : i - 1;
  } while (!settings.pages[i].IsDefined());
  return i;
}


void
Pages::Prev()
{
  UIState &ui_state = CommonInterface::SetUIState();

  ui_state.page_index = PrevIndex();
  Update();
}

void
Pages::Open(unsigned page)
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  UIState &ui_state = CommonInterface::SetUIState();

  if (page >= PageSettings::MAX_PAGES)
    return;

  if (!settings.pages[page].IsDefined())
    return;

  ui_state.page_index = page;
  Update();
}


void
Pages::OpenLayout(const PageLayout &layout)
{
  UIState &ui_state = CommonInterface::SetUIState();

  if (!layout.valid)
    return;

  if (!layout.infobox_config.enabled) {
    CommonInterface::main_window->SetFullScreen(true);
    ui_state.auxiliary_enabled = false;
  } else {
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
  }

  switch (layout.bottom) {
  case PageLayout::Bottom::NOTHING:
    CommonInterface::main_window->SetBottomWidget(nullptr);
    break;

  case PageLayout::Bottom::CROSS_SECTION:
    CommonInterface::main_window->SetBottomWidget(new CrossSectionWidget());
    break;

  case PageLayout::Bottom::MAX:
    gcc_unreachable();
  }

  switch (layout.main) {
  case PageLayout::Main::MAP:
    CommonInterface::main_window->ActivateMap();
    break;

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();
}
