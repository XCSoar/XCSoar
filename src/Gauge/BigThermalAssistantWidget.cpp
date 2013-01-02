/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "BigThermalAssistantWidget.hpp"
#include "Gauge/BigThermalAssistantWindow.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Screen/Layout.hpp"

void
BigThermalAssistantWidget::Update(const AttitudeState &attitude,
                                  const DerivedInfo &calculated)
{
  BigThermalAssistantWindow *window =
    (BigThermalAssistantWindow *)OverlappedWidget::GetWindow();
  window->Update(attitude, calculated);
}

void
BigThermalAssistantWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();

  BigThermalAssistantWindow *window =
    new BigThermalAssistantWindow(look, Layout::FastScale(10));
  window->Create(parent, rc, style);
  SetWindow(window);
}

void
BigThermalAssistantWidget::Unprepare()
{
  BigThermalAssistantWindow *window =
    (BigThermalAssistantWindow *)OverlappedWidget::GetWindow();
  delete window;

  OverlappedWidget::Unprepare();
}

void
BigThermalAssistantWidget::Show(const PixelRect &rc)
{
  Update(blackboard.Basic().attitude, blackboard.Calculated());

  OverlappedWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
BigThermalAssistantWidget::Hide()
{
  blackboard.RemoveListener(*this);
  OverlappedWidget::Hide();
}

bool
BigThermalAssistantWidget::SetFocus()
{
  return false;
}

void
BigThermalAssistantWidget::OnCalculatedUpdate(const MoreData &basic,
                                           const DerivedInfo &calculated)
{
  Update(basic.attitude, calculated);
}
