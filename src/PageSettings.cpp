// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageSettings.hpp"
#include "PageOverlayTitle.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"
#include "util/StringBuilder.hxx"
#include "util/UTF8.hpp"

#include <algorithm>
#include <cassert>

void
PageLayout::FormatInfoTitle(std::span<char> buffer,
                            const InfoBoxSettings &info_box_settings,
                            bool concise) const noexcept
{
  if (buffer.empty())
    return;

  buffer.front() = '\0';
  BasicStringBuilder<char> builder{buffer};

  try {
    if (infobox_config.enabled) {
      builder.Append(concise ? _("Info") : _("Map and InfoBoxes"));

      if (!infobox_config.auto_switch &&
          infobox_config.panel < InfoBoxSettings::MAX_PANELS) {
        builder.Append(' ');
        builder.Append(gettext(info_box_settings.panels[infobox_config.panel].name));
      } else if (concise) {
        builder.Append(' ');
        builder.Append(C_("Status", "Auto"));
      } else {
        builder.Append(" (");
        builder.Append(C_("Status", "Auto"));
        builder.Append(')');
      }
    } else if (concise) {
      builder.Append(_("Info Hide"));
    } else {
      builder.Append(_("Map (Full screen)"));
    }
  } catch (BasicStringBuilder<char>::Overflow) {
    CropIncompleteUTF8(buffer.data());
  }
}

const char *
PageLayout::MakeTitle(const InfoBoxSettings &info_box_settings,
                      std::span<char> buffer,
                      const RaspStore *rasp,
                      const bool concise) const noexcept
{
  if (!valid)
    return "---";

  switch (main) {
  case PageLayout::Main::MAP:
  case PageLayout::Main::MAP_NORTH_UP:
  case PageLayout::Main::EDL_MAP:
    break;

  case PageLayout::Main::FLARM_RADAR:
    return _("FLARM Radar");

  case PageLayout::Main::THERMAL_ASSISTANT:
    return _("Thermal Assistant");

  case PageLayout::Main::HORIZON:
    return _("Horizon");

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  assert(!buffer.empty());
  /* Write the InfoBox title straight into the caller's buffer.  A
     later piece that does not fit is skipped, so a short page-list
     row keeps "Map and InfoBoxes" instead of going blank. */
  FormatInfoTitle(buffer, info_box_settings, concise);

  BasicStringBuilder<char> builder{buffer};
  builder.Extend(StringLength(buffer.data()));

  try {
    char label[128];
    if (FormatOverlayLabel({label, std::size(label)}, *this, rasp)) {
      const char *sep = overlay == PageLayout::Overlay::SKYSIGHT
        ? " | " : ", ";
      if (builder.CanAppend(StringLength(sep) + StringLength(label))) {
        builder.Append(sep);
        builder.Append(label);
      }
    }

    switch (bottom) {
    case Bottom::NOTHING:
    case Bottom::CUSTOM:
      break;

    case Bottom::CROSS_SECTION:
      builder.Append(", XS");
      break;

    case Bottom::WEATHER_CONTROLS:
      break;

    case Bottom::NAVIGATOR:
      builder.Append(", NAV_B");
      break;
    case Bottom::NAVIGATOR_LITE_ONE_LINE:
      builder.Append(", NAV_Lite_1L_B");
      break;
    case Bottom::NAVIGATOR_LITE_TWO_LINES:
      builder.Append(", NAV_Lite_2L_B");
      break;
    case Bottom::NAVIGATOR_DETAILED:
      builder.Append(", NAV_Detailed_B");
      break;
      
    case Bottom::MAX:
      gcc_unreachable();
    }

    switch (top) {
    case Top::NOTHING:
    case Top::CUSTOM:
      break;
    case Top::NAVIGATOR:
      builder.Append(", NAV");
      break;
    case Top::NAVIGATOR_LITE_ONE_LINE:
      builder.Append(", NAV_Lite_1L");
      break;
    case Top::NAVIGATOR_LITE_TWO_LINES:
      builder.Append(", NAV_Lite_2L");
      break;
    case Top::NAVIGATOR_DETAILED:
      builder.Append(", NAV_Detailled");
      break;
    case Top::MAX:
      gcc_unreachable();
    }
  } catch (BasicStringBuilder<char>::Overflow) {
    CropIncompleteUTF8(buffer.data());
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
