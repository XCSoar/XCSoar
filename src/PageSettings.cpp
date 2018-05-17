/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "PageSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"

#include <algorithm>

void
PageLayout::MakeTitle(const InfoBoxSettings &info_box_settings,
                      TCHAR *buffer, const bool concise) const
{
  if (!valid) {
    _tcscpy(buffer, _T("---"));
    return;
  }

  switch (main) {
  case PageLayout::Main::MAP:
    break;

  case PageLayout::Main::FLARM_RADAR:
    _tcscpy(buffer, _("FLARM radar"));
    return;

  case PageLayout::Main::THERMAL_ASSISTANT:
    _tcscpy(buffer, _("Thermal assistant"));
    return;

  case PageLayout::Main::HORIZON:
    _tcscpy(buffer, _("Horizon"));
    return;

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  if (infobox_config.enabled) {
    _tcscpy(buffer, concise ? _("Info") : _("Map and InfoBoxes"));

    if (!infobox_config.auto_switch &&
        infobox_config.panel < InfoBoxSettings::MAX_PANELS) {
      _tcscat(buffer, _T(" "));
      _tcscat(buffer,
              gettext(info_box_settings.panels[infobox_config.panel].name));
    }
    else {
      if (concise) {
        _tcscat(buffer, _T(" "));
        _tcscat(buffer, _("Auto"));
      } else {
        _tcscat(buffer, _T(" ("));
        _tcscat(buffer, _("Auto"));
        _tcscat(buffer, _T(")"));
      }
    }
  } else {
    if (concise)
      _tcscpy(buffer, _("Info Hide"));
    else
      _tcscpy(buffer, _("Map (Full screen)"));
  }

  switch (bottom) {
  case Bottom::NOTHING:
  case Bottom::CUSTOM:
    break;

  case Bottom::CROSS_SECTION:
    // TODO: better text and translate
    _tcscat(buffer, _T(", XS"));
    break;

  case Bottom::MAX:
    gcc_unreachable();
  }
}

void
PageSettings::SetDefaults()
{
  pages[0] = PageLayout::Default();
  pages[1] = PageLayout::FullScreen();

  std::fill(pages.begin() + 2, pages.end(), PageLayout::Undefined());

  n_pages = 2;

  distinct_zoom = false;
}

void
PageSettings::Compress()
{
  auto last = std::remove_if(pages.begin(), pages.end(),
                             [](const PageLayout &layout) {
                               return !layout.IsDefined();
                             });
  std::fill(last, pages.end(), PageLayout::Undefined());
  n_pages = std::distance(pages.begin(), last);
}
