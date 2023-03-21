// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BigThermalAssistantWidget.hpp"
#include "Gauge/BigThermalAssistantWindow.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Language/Language.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"

BigThermalAssistantWidget::BigThermalAssistantWidget(LiveBlackboard &_blackboard,
                                                     const ThermalAssistantLook &_look) noexcept
    :blackboard(_blackboard), look(_look) {}

BigThermalAssistantWidget::~BigThermalAssistantWidget() noexcept = default;

void
BigThermalAssistantWidget::UpdateLayout() noexcept
{
  const PixelRect rc = GetContainer().GetClientRect();
  view->Move(rc);

  const unsigned margin = Layout::Scale(1);
  const unsigned button_height = Layout::GetMinimumControlHeight();

  PixelRect button_rc;
  button_rc.bottom = rc.bottom - margin;
  button_rc.top = button_rc.bottom - button_height;
  button_rc.right = rc.right - margin;
  button_rc.left = button_rc.right - Layout::Scale(50);
  close_button->Move(button_rc);
}

void
BigThermalAssistantWidget::Update(const AttitudeState &attitude,
                                  const DerivedInfo &calculated) noexcept
{
  view->Update(attitude, calculated);
}

void
BigThermalAssistantWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &_rc) noexcept
{
  ContainerWidget::Prepare(parent, _rc);

  const PixelRect rc = GetContainer().GetClientRect();

  close_button = std::make_unique<Button>(GetContainer(),
                                          UIGlobals::GetDialogLook().button,
                                          _("Close"), rc, WindowStyle(),
                                          [](){ PageActions::Restore(); });

  view = std::make_unique<BigThermalAssistantWindow>(look,
                                                     Layout::FastScale(10));
  view->Create(GetContainer(), rc);
}

void
BigThermalAssistantWidget::Show(const PixelRect &rc) noexcept
{
  Update(blackboard.Basic().attitude, blackboard.Calculated());

  ContainerWidget::Show(rc);
  UpdateLayout();

  /* show the "Close" button only if this is a "special" page */
  close_button->SetVisible(CommonInterface::GetUIState().pages.special_page.IsDefined());

  blackboard.AddListener(*this);
}

void
BigThermalAssistantWidget::Hide() noexcept
{
  blackboard.RemoveListener(*this);
  ContainerWidget::Hide();
}

void
BigThermalAssistantWidget::Move(const PixelRect &rc) noexcept
{
  ContainerWidget::Move(rc);

  UpdateLayout();
}

bool
BigThermalAssistantWidget::SetFocus() noexcept
{
  return false;
}

void
BigThermalAssistantWidget::OnCalculatedUpdate(const MoreData &basic,
                                           const DerivedInfo &calculated)
{
  Update(basic.attitude, calculated);
}
