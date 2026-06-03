// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardViewWidget.hpp"
#include "ForwardView/ForwardViewWindow.hpp"
#include "ForwardView/ForwardViewGeometry.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "DataComponents.hpp"

#include <algorithm>

void
ForwardViewWidget::Update(const MoreData &basic,
                          const DerivedInfo &calculated,
                          const MapSettings &settings) noexcept
{
  ForwardViewWindow &w = (ForwardViewWindow &)GetWindow();

  w.ReadBlackboard(basic, calculated,
                   CommonInterface::GetComputerSettings().task.glide,
                   CommonInterface::GetComputerSettings().polar.glide_polar_task,
                   settings,
                   CommonInterface::GetComputerSettings().utc_offset);

  if (basic.location_available && basic.track_available) {
    const double range = std::clamp(ForwardViewGeometry::VIEW_RANGE_DEFAULT,
                                    ForwardViewGeometry::VIEW_RANGE_MIN,
                                    ForwardViewGeometry::VIEW_RANGE_MAX);
    const double speed = basic.ground_speed_available ? basic.ground_speed : 0.;
    if (ForwardViewGeometry::SMOOTH_MOTION_ENABLED)
      w.SetMotionTarget(basic.location, basic.track, speed, range);
    else
      w.SetView(basic.location, basic.track, range);
  } else
    w.SetInvalid();

  w.Invalidate();
}

void
ForwardViewWidget::OnSmoothTimer() noexcept
{
  if (!ForwardViewGeometry::SMOOTH_MOTION_ENABLED || !IsDefined())
    return;

  ForwardViewWindow &w = (ForwardViewWindow &)GetWindow();
  if (w.TickMotionForRepaint())
    w.Invalidate();
}

void
ForwardViewWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();
  style.Disable();

  auto w = std::make_unique<ForwardViewWindow>(look.cross_section,
                                               look.map.airspace,
                                               look.map.topography,
                                               look.info_box.inverse);
  w->SetAirspaces(data_components.airspaces.get());
  w->SetTerrain(data_components.terrain.get());
  w->SetTopography(data_components.topography.get());
  w->Create(parent, rc, style);

  SetWindow(std::move(w));
}

void
ForwardViewWidget::Show(const PixelRect &rc) noexcept
{
  Update(CommonInterface::Basic(), CommonInterface::Calculated(),
         CommonInterface::GetMapSettings());
  CommonInterface::GetLiveBlackboard().AddListener(*this);

  if (ForwardViewGeometry::SMOOTH_MOTION_ENABLED)
    smooth_timer.Schedule(ForwardViewGeometry::SMOOTH_MOTION_INTERVAL);

  WindowWidget::Show(rc);
}

void
ForwardViewWidget::Hide() noexcept
{
  WindowWidget::Hide();

  if (ForwardViewGeometry::SMOOTH_MOTION_ENABLED)
    smooth_timer.Cancel();

  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
ForwardViewWidget::OnCalculatedUpdate(const MoreData &basic,
                                      const DerivedInfo &calculated)
{
  Update(basic, calculated, CommonInterface::GetMapSettings());
}
