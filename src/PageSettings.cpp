// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageSettings.hpp"
#include "PageOverlayTitle.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Language/Language.hpp"
#include "util/StringBuilder.hxx"
#include "util/UTF8.hpp"

#include <algorithm>
#include <cassert>

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
  /* Callers often pass an uninitialized StaticString buffer.  Start
     with an empty C string so Overflow before the first Append does
     not return stack garbage to CalcTextSize. */
  buffer.front() = '\0';

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
          builder.Append(C_("Status", "Auto"));
        } else {
          builder.Append(" (");
          builder.Append(C_("Status", "Auto"));
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

    case Bottom::WEATHER_CONTROLS:
      break;

    case Bottom::MAX:
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

  for (auto &o : overrides)
    o.Clear();

  n_pages = 2;

  distinct_zoom = true;
}

void
PageSettings::Compress() noexcept
{
  unsigned write = 0;
  for (unsigned read = 0; read < MAX_PAGES; ++read) {
    if (!pages[read].IsDefined())
      continue;

    if (write != read) {
      pages[write] = pages[read];
      overrides[write] = overrides[read];
    }
    ++write;
  }

  for (unsigned i = write; i < MAX_PAGES; ++i) {
    pages[i] = PageLayout::Undefined();
    overrides[i].Clear();
  }

  n_pages = write;
}
