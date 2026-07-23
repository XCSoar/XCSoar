// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ControlsFactory.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_EDL
#include "EdlControlsModel.hpp"
#endif
#include "RaspControlsModel.hpp"
#ifdef HAVE_HTTP
#include "DataGlobals.hpp"
#include "SkySightControlsModel.hpp"
#endif
#include "XcthermControlsModel.hpp"

#include <memory>

namespace WeatherMapOverlay {

std::unique_ptr<ControlsModel>
CreateControlsModel(const PageLayout &layout) noexcept
{
  switch (layout.overlay) {
  case PageLayout::Overlay::EDL:
#ifdef HAVE_EDL
    return std::make_unique<EdlControlsModel>();
#else
    break;
#endif

  case PageLayout::Overlay::RASP:
    return std::make_unique<RaspControlsModel>();

  case PageLayout::Overlay::XCTHERM:
    return std::make_unique<XcthermControlsModel>();

  case PageLayout::Overlay::SKYSIGHT:
#ifdef HAVE_HTTP
    if (const auto skysight = DataGlobals::GetSkySight(); skysight != nullptr)
      return std::make_unique<SkySightControlsModel>(
        skysight, layout.skysight_overlay.c_str());
    break;
#else
    break;
#endif

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }

  return nullptr;
}

} // namespace WeatherMapOverlay
