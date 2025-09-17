// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgOnboarding.hpp"
#include "WelcomeWidget.hpp"
#include "GestureHelpWidget.hpp"
#include "ConfigurationWidget.hpp"
#include "PreflightWidget.hpp"
#include "PostflightWidget.hpp"
#include "DontShowAgainWidget.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Language/Language.hpp"

#include <array>

void
dlgOnboardingShowModal()
{
  const std::array<const TCHAR*, 6> titles = {
    _("Getting started: Welcome"),
    _("Getting started: Gestures"),
    _("Getting started: Configuration"),
    _("Getting started: Preflight"),
    _("Getting started: Postflight"),
    _("Getting started: Show again"),
  };

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Getting started"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button, dialog.MakeModalResultCallback(mrOK));

  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<WelcomeWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<GestureHelpWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<ConfigurationWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<PreflightWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<PostflightWidget>(), look));
  pager->Add(std::make_unique<DontShowAgainWidget>(look));

  ArrowPagerWidget *p = pager.get();
  pager->SetPageFlippedCallback([&dialog, &titles, p](){
    dialog.SetCaption(titles[p->GetCurrentIndex()]);
  });

  dialog.SetCaption(titles[pager->GetCurrentIndex()]);
  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
