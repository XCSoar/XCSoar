// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "util/StringBuilder.hxx"

#include <algorithm>

void
PageLayout::Normalise() noexcept
{
  if (main == Main::EDL_MAP) {
    main = Main::MAP;
    overlay = Overlay::EDL;
    if (bottom == Bottom::NOTHING)
      bottom = Bottom::EDL_CONTROLS;
  }

  if (unsigned(overlay) >= unsigned(Overlay::MAX))
    overlay = Overlay::NONE;

  if (IsMapMain()) {
    if (UsesWeatherOverlay()) {
      if (bottom == Bottom::NOTHING)
        bottom = Bottom::EDL_CONTROLS;
    } else if (overlay == Overlay::NONE &&
               bottom == Bottom::EDL_CONTROLS)
      bottom = Bottom::NOTHING;
  } else {
    overlay = Overlay::NONE;
    if (bottom == Bottom::EDL_CONTROLS)
      bottom = Bottom::NOTHING;
  }

  if (overlay != Overlay::RASP)
    rasp_field = -1;
  else if (rasp_field < 0)
    rasp_field = 0;
}

static void
AppendOverlayTitle(BasicStringBuilder<char> &builder,
                   const PageLayout &layout,
                   const RaspStore *rasp)
{
  switch (layout.overlay) {
  case PageLayout::Overlay::NONE:
    break;

  case PageLayout::Overlay::RASP:
    builder.Append(", RASP");
    if (rasp != nullptr &&
        layout.rasp_field >= 0 &&
        unsigned(layout.rasp_field) < rasp->GetItemCount()) {
      const auto &item = rasp->GetItemInfo(layout.rasp_field);
      const char *label = item.label != nullptr
        ? gettext(item.label)
        : item.name;
      if (label != nullptr && *label != '\0') {
        builder.Append(' ');
        builder.Append(label);
      }
    }
    break;

  case PageLayout::Overlay::EDL:
    builder.Append(", EDL");
    break;

  case PageLayout::Overlay::MAX:
    gcc_unreachable();
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
    return _("FLARM radar");

  case PageLayout::Main::THERMAL_ASSISTANT:
    return _("Thermal assistant");

  case PageLayout::Main::HORIZON:
    return _("Horizon");

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  BasicStringBuilder<char> builder{buffer};

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
          builder.Append(" (");
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

    AppendOverlayTitle(builder, *this, rasp);

    switch (bottom) {
    case Bottom::NOTHING:
    case Bottom::CUSTOM:
      break;

    case Bottom::CROSS_SECTION:
      builder.Append(", XS");
      break;

    case Bottom::EDL_CONTROLS:
      break;

    case Bottom::MAX:
      gcc_unreachable();
    }
  } catch (BasicStringBuilder<char>::Overflow) {
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
