/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "CrossSectionWidget.hpp"
#include "CrossSection/CrossSectionWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "util/Clamp.hpp"

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
    w.SetRange(Clamp(double(w.GetWidth()) / settings.cruise_scale,
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
  w->SetAirspaces(&airspace_database);
  w->SetTerrain(terrain);
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
