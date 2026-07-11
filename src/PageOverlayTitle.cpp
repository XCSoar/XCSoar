// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageOverlayTitle.hpp"

#include "Language/Language.hpp"
#include "Weather/Features.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "util/StaticString.hxx"
#include "util/StringBuilder.hxx"
#include "util/Compiler.h"

#ifdef HAVE_EDL
#include "Formatter/UserUnits.hpp"
#include "Weather/EDL/StateController.hpp"
#endif
#ifdef HAVE_HTTP
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#endif

void
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
#ifdef HAVE_EDL
    {
      EDL::EnsureInitialised();
      char alt[32];
      FormatUserAltitude(EDL::GetAltitude(), alt);
      if (alt[0] != '\0') {
        builder.Append(' ');
        builder.Append(alt);
      }
    }
#endif
    break;

  case PageLayout::Overlay::XCTHERM:
    builder.Append(", XCTherm");
#ifdef HAVE_HTTP
    {
      StaticString<64> label;
      XCTherm::FormatLayerTitleLabel(label);
      if (!label.empty()) {
        builder.Append(' ');
        builder.Append(label.c_str());
      }
    }
#endif
    break;

  case PageLayout::Overlay::MAX:
    gcc_unreachable();
  }
}
