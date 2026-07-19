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
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/StateController.hpp"
#endif
#ifdef HAVE_HTTP
#include "Interface.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
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
      if (layout.edl_isobar > 0 &&
          EDL::IsSupportedIsobar(unsigned(layout.edl_isobar)))
        FormatUserAltitude(
          EDL::GetAltitudeForIsobar(unsigned(layout.edl_isobar)), alt);
      else
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
      builder.Append(' ');
      if (layout.xctherm_layer == PageLayout::XCTHERM_LAYER_AUTO)
        builder.Append(_("Auto"));
      else {
        const auto &settings =
          CommonInterface::GetComputerSettings().weather.xctherm;
        const auto &region = XCTherm::GetRegion(settings.model);
        if (layout.xctherm_layer >= 0 &&
            unsigned(layout.xctherm_layer) < region.layer_count)
          builder.Append(
            gettext(region.layers[layout.xctherm_layer].short_label));
      }
    }
#endif
    break;

  case PageLayout::Overlay::MAX:
    gcc_unreachable();
  }
}
