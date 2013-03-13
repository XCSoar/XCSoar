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
#include "Language/Language.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"

void
BigThermalAssistantWidget::UpdateLayout()
{
  const PixelRect rc = GetContainer().GetClientRect();
  view->Move(rc);

#ifndef GNAV
  const unsigned margin = Layout::Scale(1);
  const unsigned button_height = Layout::GetMinimumControlHeight();

  PixelRect button_rc;
  button_rc.bottom = rc.bottom - margin;
  button_rc.top = button_rc.bottom - button_height;
  button_rc.right = rc.right - margin;
  button_rc.left = button_rc.right - Layout::Scale(50);
  close_button->Move(button_rc);
#endif
}

void
BigThermalAssistantWidget::Update(const AttitudeState &attitude,
                                  const DerivedInfo &calculated)
{
  view->Update(attitude, calculated);
}

void
BigThermalAssistantWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &_rc)
{
  ContainerWidget::Prepare(parent, _rc);

  const PixelRect rc = GetContainer().GetClientRect();

#ifndef GNAV
  close_button = new WndButton(GetContainer(), UIGlobals::GetDialogLook(),
                               _("Close"), rc, ButtonWindowStyle(),
                               *this, CLOSE);
#endif

  view = new BigThermalAssistantWindow(look, Layout::FastScale(10));
  view->Create(GetContainer(), rc);
}

void
BigThermalAssistantWidget::Unprepare()
{
  delete view;
#ifndef GNAV
  delete close_button;
#endif

  ContainerWidget::Unprepare();
}

void
BigThermalAssistantWidget::Show(const PixelRect &rc)
{
  Update(blackboard.Basic().attitude, blackboard.Calculated());

  ContainerWidget::Show(rc);
  UpdateLayout();

#ifndef GNAV
  /* show the "Close" button only if this is a "special" page */
  close_button->SetVisible(CommonInterface::GetUIState().pages.special_page.IsDefined());
#endif

  blackboard.AddListener(*this);
}

void
BigThermalAssistantWidget::Hide()
{
  blackboard.RemoveListener(*this);
  ContainerWidget::Hide();
}

void
BigThermalAssistantWidget::Move(const PixelRect &rc)
{
  ContainerWidget::Move(rc);

  UpdateLayout();
}

bool
BigThermalAssistantWidget::SetFocus()
{
  return false;
}

#ifndef GNAV

void
BigThermalAssistantWidget::OnAction(int id)
{
  switch ((Action)id) {
  case CLOSE:
    PageActions::Restore();
    break;
  }
}

#endif

void
BigThermalAssistantWidget::OnCalculatedUpdate(const MoreData &basic,
                                           const DerivedInfo &calculated)
{
  Update(basic.attitude, calculated);
}
