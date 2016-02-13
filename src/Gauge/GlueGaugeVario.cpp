/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Gauge/GlueGaugeVario.hpp"
#include "Gauge/GaugeVario.hpp"
#include "Blackboard/LiveBlackboard.hpp"

void
GlueGaugeVario::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();
  style.Disable();

  GaugeVario *gauge = new GaugeVario(blackboard, parent, look,
                                     rc, style);
  SetWindow(gauge);
}

void
GlueGaugeVario::Unprepare()
{
  DeleteWindow();
}

void
GlueGaugeVario::Show(const PixelRect &rc)
{
  WindowWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
GlueGaugeVario::Hide()
{
  blackboard.RemoveListener(*this);

  WindowWidget::Hide();
}

void
GlueGaugeVario::OnGPSUpdate(const MoreData &basic)
{
  ((GaugeVario &)GetWindow()).Invalidate();
}
