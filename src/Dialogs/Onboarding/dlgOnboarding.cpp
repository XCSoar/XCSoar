// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgOnboarding.hpp"
#include "WelcomeWidget.hpp"
#include "GestureHelpWidget.hpp"
#include "ConfigurationWidget.hpp"
#include "DontShowAgainWidget.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Language/Language.hpp"

void
dlgOnboardingShowModal()
{
  std::vector titles = {
    _("Getting started: Welcome"),
    _("Getting started: Gestures"),
    _("Getting started: Configuration"),
    _("Getting started: Show again"),
  };

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Getting started"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button, dialog.MakeModalResultCallback(mrOK));

  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<WelcomeWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<GestureHelpWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<ConfigurationWidget>(), look));
  pager->Add(std::make_unique<DontShowAgainWidget>(look));

  pager->SetPageFlippedCallback([&dialog, &titles, &pager=*pager](){
    dialog.SetCaption(titles[pager.GetCurrentIndex()]);
  });

  dialog.SetCaption(titles[pager->GetCurrentIndex()]);

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
