// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgGestureHelp.hpp"
#include "WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Widget/RichTextWidget.hpp"
#include "Language/Language.hpp"

const char gesture_help_text[] =
  "# Gesture Navigation\n\n"
  "XCSoar supports touch gestures on the map:\n\n"
  "- ![R](resource:IDB_GESTURE_RIGHT) **Swipe Right** (R): Zoom in\n"
  "- ![L](resource:IDB_GESTURE_LEFT) **Swipe Left** (L): Zoom out\n"
  "- ![U](resource:IDB_GESTURE_UP) **Swipe Up** (U): Previous InfoBox set / Zoom in\n"
  "- ![D](resource:IDB_GESTURE_DOWN) **Swipe Down** (D): Next InfoBox set / Zoom out\n"
  "- ![UD](resource:IDB_GESTURE_UD) **Swipe Up-Down** (UD): Full-screen toggle\n"
  "- ![DU](resource:IDB_GESTURE_DU) **Swipe Down-Up** (DU): Toggle wind arrow\n"
  "- ![LR](resource:IDB_GESTURE_LDRDL) **Swipe Left-Right** (LR): Toggle thermal assistant\n"
  "- ![RL](resource:IDB_GESTURE_RL) **Swipe Right-Left** (RL): Analysis\n"
  "- ![DR](resource:IDB_GESTURE_DR) **Swipe Down-Right** (DR): Waypoint list\n"
  "- ![DL](resource:IDB_GESTURE_DL) **Swipe Down-Left** (DL): Alternates\n"
  "- ![UL](resource:IDB_GESTURE_ULDR) **Swipe Up-Left** (UL): Task manager\n"
  "- ![UR](resource:IDB_GESTURE_URD) **Swipe Up-Right** (UR): Target dialog\n\n"
  "Long-press on the map to enter Pan mode. "
  "Tap while in Pan mode to see nearby waypoints.";

void
dlgGestureHelpShowModal() noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      look, _("Gesture Help"));

  dialog.AddButton(_("Close"), mrOK);

  auto content = std::make_unique<VScrollWidget>(
    std::make_unique<RichTextWidget>(look, gesture_help_text),
    look, true);

  dialog.FinishPreliminary(std::move(content));
  dialog.ShowModal();
}
