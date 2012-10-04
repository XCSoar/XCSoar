/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

void
CrossSectionWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();

  CrossSectionWindow *w =
    new CrossSectionWindow(look.cross_section, look.map.airspace, look.chart);
  w->SetAirspaces(&airspace_database);
  w->SetTerrain(terrain);
  w->Create(parent, rc, style);

  SetWindow(w);

  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
CrossSectionWidget::Unprepare()
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  DeleteWindow();
}

void
CrossSectionWidget::OnCalculatedUpdate(const MoreData &basic,
                                       const DerivedInfo &calculated)
{
  CrossSectionWindow &w = *(CrossSectionWindow *)GetWindow();

  w.ReadBlackboard(basic, calculated,
                   CommonInterface::GetMapSettings().airspace);

  if (basic.location_available && basic.track_available) {
    w.SetStart(basic.location);
    w.SetDirection(basic.track);
  } else
    w.SetInvalid();

  w.Invalidate();
}
