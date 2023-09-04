// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CrossSectionWidget.hpp"
#include "CrossSection/CrossSectionWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "DataComponents.hpp"

#include <algorithm> // for std::clamp()

void
CrossSectionWidget::Update(const MoreData &basic,
                           const DerivedInfo &calculated,
                           const MapSettings &settings) noexcept
{
  CrossSectionWindow &w = (CrossSectionWindow &)GetWindow();

  w.ReadBlackboard(basic, calculated,
                   CommonInterface::GetComputerSettings().task.glide,
                   CommonInterface::GetComputerSettings().polar.glide_polar_task,
                   CommonInterface::GetMapSettings());

  if (basic.location_available && basic.track_available) {
    w.SetStart(basic.location);
    w.SetDirection(basic.track);
    w.SetRange(std::clamp(double(w.GetSize().width) / settings.cruise_scale,
                          5000., 200000.));
  } else
    w.SetInvalid();

  w.Invalidate();
}

void
CrossSectionWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();
  style.Disable();

  auto w =
    std::make_unique<CrossSectionWindow>(look.cross_section,
                                         look.map.airspace,
                                         look.chart, look.info_box);
  w->SetAirspaces(data_components.airspaces.get());
  w->SetTerrain(data_components.terrain.get());
  w->Create(parent, rc, style);

  SetWindow(std::move(w));
}

void
CrossSectionWidget::Show(const PixelRect &rc) noexcept
{
  Update(CommonInterface::Basic(), CommonInterface::Calculated(),
         CommonInterface::GetMapSettings());
  CommonInterface::GetLiveBlackboard().AddListener(*this);

  WindowWidget::Show(rc);
}

void
CrossSectionWidget::Hide() noexcept
{
  WindowWidget::Hide();

  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
CrossSectionWidget::OnCalculatedUpdate(const MoreData &basic,
                                       const DerivedInfo &calculated)
{
  Update(basic, calculated, CommonInterface::GetMapSettings());
}
