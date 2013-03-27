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

#include "BigTrafficWidget.hpp"
#include "Screen/Layout.hpp"
#include "Form/SymbolButton.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#include "PageActions.hpp"
#include "Look/Look.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "FlarmTrafficControl.hpp"
#include "Input/InputEvents.hpp"

void
TrafficWidget::OpenDetails()
{
  view->OpenDetails();
}

void
TrafficWidget::ZoomIn()
{
  view->ZoomIn();
  UpdateButtons();
}

void
TrafficWidget::ZoomOut()
{
  view->ZoomOut();
  UpdateButtons();
}

void
TrafficWidget::PreviousTarget()
{
  view->PrevTarget();
}

void
TrafficWidget::NextTarget()
{
  view->NextTarget();
}

void
TrafficWidget::SwitchData()
{
  view->SwitchData();
}

bool
TrafficWidget::GetAutoZoom() const
{
  return view->GetAutoZoom();
}

void
TrafficWidget::SetAutoZoom(bool value)
{
  view->SetAutoZoom(value);
}

void
TrafficWidget::ToggleAutoZoom()
{
  view->ToggleAutoZoom();
}

bool
TrafficWidget::GetNorthUp() const
{
  return view->GetNorthUp();
}

void
TrafficWidget::SetNorthUp(bool value)
{
  view->SetAutoZoom(value);
}

void
TrafficWidget::ToggleNorthUp()
{
  view->ToggleNorthUp();
}

static bool
OnMouseDouble(PixelScalar x, PixelScalar y)
{
  InputEvents::ShowMenu();
  return true;
}

void
TrafficWidget::Update()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (CommonInterface::GetUISettings().traffic.auto_close_dialog &&
      basic.flarm.traffic.IsEmpty()) {
    /* this must be deferred, because this method is called from
       within the BlackboardListener, and we must not unregister the
       listener in this context */
    PageActions::DeferredRestore();
    return;
  }

  view->Update(basic.track,
               basic.flarm.traffic,
               CommonInterface::GetComputerSettings().team_code);

  view->UpdateTaskDirection(calculated.task_stats.task_valid &&
                            calculated.task_stats.current_leg.solution_remaining.IsOk(),
                            calculated.task_stats.
                            current_leg.solution_remaining.cruise_track_bearing);

  UpdateButtons();
}

void
TrafficWidget::UpdateLayout()
{
  const PixelRect rc = GetContainer().GetClientRect();
  view->Move(rc);

#ifndef GNAV
  const unsigned margin = Layout::Scale(1);
  const unsigned button_height = Layout::GetMinimumControlHeight();
  const unsigned button_width = std::max(unsigned(rc.right / 6),
                                         button_height);

  const PixelScalar x1 = rc.right / 2;
  const PixelScalar x0 = x1 - button_width;
  const PixelScalar x2 = x1 + button_width;

  const int y0 = margin;
  const int y1 = y0 + button_height;
  const int y3 = rc.bottom - margin;
  const int y2 = y3 - button_height;

  PixelRect button_rc;

  button_rc.left = x0;
  button_rc.top = y0;
  button_rc.right = x1 - margin;
  button_rc.bottom = y1;
  zoom_in_button->Move(button_rc);

  button_rc.left = x1;
  button_rc.right = x2 - margin;
  zoom_out_button->Move(button_rc);

  button_rc.left = x0;
  button_rc.top = y2;
  button_rc.right = x1 - margin;
  button_rc.bottom = y3;
  previous_item_button->Move(button_rc);

  button_rc.left = x1;
  button_rc.right = x2 - margin;
  next_item_button->Move(button_rc);

  button_rc.left = margin;
  button_rc.top = button_height * 3 / 2;
  button_rc.right = button_rc.left + Layout::Scale(50);
  button_rc.bottom = button_rc.top + button_height;
  details_button->Move(button_rc);

  button_rc.right = rc.right - margin;
  button_rc.left = button_rc.right - Layout::Scale(50);
  close_button->Move(button_rc);
#endif
}

void
TrafficWidget::UpdateButtons()
{
#ifndef GNAV
  const bool unlocked = !view->WarningMode();
  const TrafficList &traffic = CommonInterface::Basic().flarm.traffic;
  const bool not_empty = !traffic.IsEmpty();
  const bool two_or_more = traffic.GetActiveTrafficCount() >= 2;

  zoom_in_button->SetEnabled(unlocked && view->CanZoomIn());
  zoom_out_button->SetEnabled(unlocked && view->CanZoomOut());
  previous_item_button->SetEnabled(unlocked && two_or_more);
  next_item_button->SetEnabled(unlocked && two_or_more);
  details_button->SetEnabled(unlocked && not_empty);
#endif
}

void
TrafficWidget::Prepare(ContainerWindow &parent, const PixelRect &_rc)
{
  ContainerWidget::Prepare(parent, _rc);

  const Look &look = UIGlobals::GetLook();

  const PixelRect rc = GetContainer().GetClientRect();

#ifndef GNAV
  zoom_in_button = new WndSymbolButton(GetContainer(), look.dialog,
                                       _T("+"), rc, ButtonWindowStyle(),
                                       *this, ZOOM_IN);
  zoom_out_button = new WndSymbolButton(GetContainer(), look.dialog,
                                        _T("-"), rc, ButtonWindowStyle(),
                                        *this, ZOOM_OUT);
  previous_item_button = new WndSymbolButton(GetContainer(), look.dialog,
                                             _T("<"), rc, ButtonWindowStyle(),
                                             *this, PREVIOUS_ITEM);
  next_item_button = new WndSymbolButton(GetContainer(), look.dialog,
                                         _T(">"), rc, ButtonWindowStyle(),
                                         *this, NEXT_ITEM);
  details_button = new WndButton(GetContainer(), look.dialog,
                                 _("Details"), rc, ButtonWindowStyle(),
                                 *this, DETAILS);
  close_button = new WndButton(GetContainer(), look.dialog,
                               _("Close"), rc, ButtonWindowStyle(),
                               *this, CLOSE);
#endif

  WindowStyle style;
  style.EnableDoubleClicks();

  view = new FlarmTrafficControl(look.flarm_dialog);
  view->Create(GetContainer(), rc, style);
  view->SetMouseDoubleFunction(OnMouseDouble);

  UpdateLayout();
}

void
TrafficWidget::Unprepare()
{
#ifndef GNAV
  delete zoom_in_button;
  delete zoom_out_button;
  delete previous_item_button;
  delete next_item_button;
  delete details_button;
  delete close_button;
#endif

  delete view;

  ContainerWidget::Unprepare();
}

void
TrafficWidget::Show(const PixelRect &rc)
{
  // Update Radar and Selection for the first time
  Update();

  ContainerWidget::Show(rc);
  UpdateLayout();

#ifndef GNAV
  /* show the "Close" button only if this is a "special" page */
  close_button->SetVisible(CommonInterface::GetUIState().pages.special_page.IsDefined());
#endif

  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
TrafficWidget::Hide()
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  ContainerWidget::Hide();
}

void
TrafficWidget::Move(const PixelRect &rc)
{
  ContainerWidget::Move(rc);

  UpdateLayout();
}


bool
TrafficWidget::SetFocus()
{
  view->SetFocus();
  return true;
}

#ifndef GNAV

void
TrafficWidget::OnAction(int id)
{
  switch ((Action)id) {
  case CLOSE:
    PageActions::Restore();
    break;

  case DETAILS:
    OpenDetails();
    break;

  case PREVIOUS_ITEM:
    PreviousTarget();
    break;

  case NEXT_ITEM:
    NextTarget();
    break;

  case ZOOM_IN:
    ZoomIn();
    break;

  case ZOOM_OUT:
    ZoomOut();
    break;
  }
}

#endif

void
TrafficWidget::OnGPSUpdate(const MoreData &basic)
{
  Update();
}
