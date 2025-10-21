// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"
#include "util/StringBuilder.hxx"

#include <algorithm>

const TCHAR *
PageLayout::MakeTitle(const InfoBoxSettings &info_box_settings,
                      std::span<TCHAR> buffer,
                      const bool concise) const noexcept
{
  if (!valid)
    return _T("---");

  switch (main) {
  case PageLayout::Main::MAP:
  case PageLayout::Main::MAP_NORTH_UP:
    break;

  case PageLayout::Main::FLARM_RADAR:
    return _("FLARM radar");

  case PageLayout::Main::THERMAL_ASSISTANT:
    return _("Thermal assistant");

  case PageLayout::Main::HORIZON:
    return _("Horizon");

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  BasicStringBuilder<TCHAR> builder{buffer};

  try {
    if (infobox_config.enabled) {
      builder.Append(concise ? _("Info") : _("Map and InfoBoxes"));

      if (!infobox_config.auto_switch &&
          infobox_config.panel < InfoBoxSettings::MAX_PANELS) {
        builder.Append(' ');
        builder.Append(gettext(info_box_settings.panels[infobox_config.panel].name));
      }
      else {
        if (concise) {
          builder.Append(' ');
          builder.Append(_("Auto"));
        } else {
          builder.Append(_T(" ("));
          builder.Append(_("Auto"));
          builder.Append(')');
        }
      }
    } else {
      if (concise)
        builder.Append(_("Info Hide"));
      else
        builder.Append(_("Map (Full screen)"));
    }

    switch (bottom) {
    case Bottom::NOTHING:
    case Bottom::CUSTOM:
      break;

    case Bottom::CROSS_SECTION:
      // TODO: better text and translate
      builder.Append(_T(", XS"));
      break;

    case Bottom::NAVIGATOR:
      builder.Append(_T(", NAV_B"));
      break;
    case Bottom::NAVIGATOR_LITE_ONE_LINE:
      builder.Append(_T(", NAV_Lite_1L_B"));
      break;
    case Bottom::NAVIGATOR_LITE_TWO_LINES:
      builder.Append(_T(", NAV_Lite_2L_B"));
      break;
    case Bottom::NAVIGATOR_DETAILED:
      builder.Append(_T(", NAV_Detailed_B"));
      break;
      
    case Bottom::MAX:
      gcc_unreachable();
    }

    switch (top) {
    case Top::NOTHING:
    case Top::CUSTOM:
      break;
    case Top::NAVIGATOR:
      builder.Append(_T(", NAV"));
      break;
    case Top::NAVIGATOR_LITE_ONE_LINE:
      builder.Append(_T(", NAV_Lite_1L"));
      break;
    case Top::NAVIGATOR_LITE_TWO_LINES:
      builder.Append(_T(", NAV_Lite_2L"));
      break;
    case Top::NAVIGATOR_DETAILED:
      builder.Append(_T(", NAV_Detailled"));
      break;
    case Top::MAX:
      gcc_unreachable();
    }
  } catch (BasicStringBuilder<TCHAR>::Overflow) {
  }

  return buffer.data();
}

void
PageSettings::SetDefaults() noexcept
{
  pages[0] = PageLayout::Default();
  pages[1] = PageLayout::FullScreen();

  std::fill(pages.begin() + 2, pages.end(), PageLayout::Undefined());

  n_pages = 2;

  distinct_zoom = true;
}

void
PageSettings::Compress() noexcept
{
  auto last = std::remove_if(pages.begin(), pages.end(),
                             [](const PageLayout &layout) {
                               return !layout.IsDefined();
                             });
  std::fill(last, pages.end(), PageLayout::Undefined());
  n_pages = std::distance(pages.begin(), last);
}
