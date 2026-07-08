// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ControlsFactory.hpp"

#ifdef HAVE_EDL
#include "EdlControlsModel.hpp"
#endif
#include "RaspControlsModel.hpp"
#include "XcthermControlsModel.hpp"

#include <memory>

namespace WeatherMapOverlay {

std::unique_ptr<ControlsModel>
CreateControlsModel(PageLayout::Overlay overlay) noexcept
{
  switch (overlay) {
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

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }

  return nullptr;
}

void
EnterOverlayPage(const PageLayout &layout) noexcept
{
  if (auto model = CreateControlsModel(layout.overlay))
    model->OnEnterPage(layout);
}

void
LeaveOverlayPage(const PageLayout &layout) noexcept
{
  if (auto model = CreateControlsModel(layout.overlay))
    model->OnLeavePage();
}

} // namespace WeatherMapOverlay
