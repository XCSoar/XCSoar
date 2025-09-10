// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DontShowAgainWidget.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"

DontShowAgainWidget::DontShowAgainWidget(const DialogLook &_look) : info_text(_look), look(_look) { }

PixelRect DontShowAgainWidget::MakeCheckboxRect(const PixelRect &rc) const noexcept {
  const unsigned cb_height = ::Layout::GetMinimumControlHeight();
  PixelRect rect;
  rect.left = rc.left;
  rect.top = rc.top;
  rect.right = rc.right;
  rect.bottom = rect.top + cb_height;
  return rect;
}

PixelRect DontShowAgainWidget::MakeTextRect(const PixelRect &rc, const PixelRect &cb_rect) const noexcept {
  PixelRect rect = rc;
  const unsigned spacing = 5;
  rect.top = cb_rect.bottom + spacing;
  return rect;
}

void DontShowAgainWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept {
  WindowStyle style;
  style.Hide();
  style.TabStop();

  checkbox.Create(parent, look, _("Don't show onboarding dialog on startup"),
    MakeCheckboxRect(rc), style,
    [](bool value) {
      Profile::Set(ProfileKeys::HideOnboardingDialogOnStartup, value);
	  Profile::Save();
    }
  );

  info_text.Prepare(parent, MakeTextRect(rc, MakeCheckboxRect(rc)));
  info_text.SetText(_("This popup can be accessed anytime from the menu under Info → Getting Started."));
}

void DontShowAgainWidget::Show(const PixelRect &rc) noexcept {
  bool hide_onboarding_dialog_on_startup = false;
  Profile::Get(ProfileKeys::HideOnboardingDialogOnStartup, hide_onboarding_dialog_on_startup);
  checkbox.SetState(hide_onboarding_dialog_on_startup);
  auto cb_rect = MakeCheckboxRect(rc);
  checkbox.MoveAndShow(cb_rect);
  info_text.Show(MakeTextRect(rc, cb_rect));
}

void DontShowAgainWidget::Hide() noexcept {
  checkbox.FastHide();
  info_text.Hide();
}

void DontShowAgainWidget::Move(const PixelRect &rc) noexcept {
  auto cb_rect = MakeCheckboxRect(rc);
  checkbox.Move(cb_rect);
  info_text.Move(MakeTextRect(rc, cb_rect));
}
