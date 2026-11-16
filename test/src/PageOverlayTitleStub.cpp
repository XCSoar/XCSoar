// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageOverlayTitle.hpp"

#include "Language/Language.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "util/StringBuilder.hxx"
#include "util/UTF8.hpp"

#include "util/Compiler.h"

bool
FormatOverlayLabel(std::span<char> buffer,
                   const PageLayout &layout,
                   const RaspStore *rasp) noexcept
{
  if (buffer.empty() || layout.overlay == PageLayout::Overlay::NONE)
    return false;

  buffer.front() = '\0';
  BasicStringBuilder<char> builder{buffer};

  try {
    switch (layout.overlay) {
    case PageLayout::Overlay::NONE:
      break;

    case PageLayout::Overlay::RASP:
      builder.Append("RASP");
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
      builder.Append("EDL");
      break;

    case PageLayout::Overlay::XCTHERM:
      builder.Append("XC Therm");
      break;

    case PageLayout::Overlay::SKYSIGHT:
      builder.Append("SkySight");
      if (!layout.skysight_overlay.empty()) {
        builder.Append(": ");
        builder.Append(layout.skysight_overlay.c_str());
      }
      break;

    case PageLayout::Overlay::RADAR:
      builder.Append(_("Radar"));
      break;

    case PageLayout::Overlay::SATELLITE:
      builder.Append(_("Satellite"));
      break;

    case PageLayout::Overlay::MAX:
      gcc_unreachable();
    }
  } catch (BasicStringBuilder<char>::Overflow) {
    CropIncompleteUTF8(buffer.data());
  }

  return buffer.front() != '\0';
}
