// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/IconLook.hpp"
#include "Look/DialogLook.hpp"
#include "StatusPanels/FlightStatusPanel.hpp"
#include "StatusPanels/TaskStatusPanel.hpp"
#include "StatusPanels/RulesStatusPanel.hpp"
#include "StatusPanels/SystemStatusPanel.hpp"
#include "StatusPanels/TimesStatusPanel.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static int status_page = 0;

static void
SetTitle(WndForm &form, const TabWidget &pager)
{
  StaticString<128> title;
  title.Format("%s: %s", _("Status"),
               pager.GetButtonCaption(pager.GetCurrentIndex()));
  form.SetCaption(title);
}

void
dlgStatusShowModal(int start_page)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Status"));

  dialog.FinishPreliminary(std::make_unique<TabWidget>(TabWidget::Orientation::AUTO,
                                                       std::make_unique<ButtonWidget>(look.button, _("Close"),
                                                                                      dialog.MakeModalResultCallback(mrOK))));
  dialog.PrepareWidget();
  auto &widget = (TabWidget &)dialog.GetWidget();

  widget.SetPageFlippedCallback([&dialog, &widget]() {
      SetTitle(dialog, widget);
    });

  const NMEAInfo &basic = CommonInterface::Basic();
  auto nearest_waypoint = basic.location_available
    ? data_components->waypoints->GetNearest(CommonInterface::Basic().location, 100000)
    : nullptr;

  /* setup tabs */

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;

  const IconLook &icons = UIGlobals::GetIconLook();
  const auto *FlightIcon = enable_icons ? &icons.hBmpTabFlight : nullptr;
  const auto *SystemIcon = enable_icons ? &icons.hBmpTabSystem : nullptr;
  const auto *TaskIcon = enable_icons ? &icons.hBmpTabTask : nullptr;
  const auto *RulesIcon = enable_icons ? &icons.hBmpTabRules : nullptr;
  const auto *TimesIcon = enable_icons ? &icons.hBmpTabTimes : nullptr;

  widget.AddTab(std::make_unique<FlightStatusPanel>(look,
                                                    std::move(nearest_waypoint)),
                _("Flight"), FlightIcon);

  widget.AddTab(std::make_unique<SystemStatusPanel>(look),
                _("System"), SystemIcon);

  widget.AddTab(std::make_unique<TaskStatusPanel>(look),
                _("Task"), TaskIcon);

  widget.AddTab(std::make_unique<RulesStatusPanel>(look),
                _("Rules"), RulesIcon);

  widget.AddTab(std::make_unique<TimesStatusPanel>(look),
                _("Times"), TimesIcon);

  /* restore previous page */

  if (start_page != -1) {
    status_page = start_page;
  }

  widget.SetCurrent(status_page);

  SetTitle(dialog, widget);

  dialog.ShowModal();

  /* save page number for next time this dialog is opened */
  status_page = widget.GetCurrentIndex();
}
