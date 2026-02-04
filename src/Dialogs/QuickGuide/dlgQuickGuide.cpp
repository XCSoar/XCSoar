// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgQuickGuide.hpp"
#include "WelcomeWidget.hpp"
#include "GestureHelpWidget.hpp"
#include "ConfigurationWidget.hpp"
#include "PreflightWidget.hpp"
#include "PostflightWidget.hpp"
#include "DontShowAgainWidget.hpp"
#include "QuickGuideScrollWidget.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

#include <array>

static std::unique_ptr<Widget>
CreateQuickGuideScrollWidget(std::unique_ptr<Widget> widget, const DialogLook &look)
{
  return std::make_unique<QuickGuideScrollWidget>(std::move(widget), look);
}

void
dlgQuickGuideShowModal()
{
  const std::array<const TCHAR*, 6> titles = {
    _("Quick Guide: Welcome"),
    _("Quick Guide: Gestures"),
    _("Quick Guide: Configuration"),
    _("Quick Guide: Preflight"),
    _("Quick Guide: Postflight"),
    _("Quick Guide: Show again"),
  };

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Quick Guide"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button, dialog.MakeModalResultCallback(mrOK));

  pager->Add(CreateQuickGuideScrollWidget(std::make_unique<WelcomeWidget>(), look));
  pager->Add(CreateQuickGuideScrollWidget(std::make_unique<GestureHelpWidget>(), look));
  pager->Add(CreateQuickGuideScrollWidget(std::make_unique<ConfigurationWidget>(), look));
  pager->Add(CreateQuickGuideScrollWidget(std::make_unique<PreflightWidget>(), look));
  pager->Add(CreateQuickGuideScrollWidget(std::make_unique<PostflightWidget>(), look));
  pager->Add(std::make_unique<DontShowAgainWidget>(look));

  ArrowPagerWidget *p = pager.get();
  const unsigned total_pages = titles.size();

  auto update_caption = [&dialog, &titles, p, total_pages]() {
    const unsigned current = p->GetCurrentIndex();
    StaticString<128> caption;
    caption.Format(_T("%s (%u/%u)"), titles[current], current + 1, total_pages);
    dialog.SetCaption(caption);
  };

  pager->SetPageFlippedCallback(update_caption);
  update_caption();
  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
