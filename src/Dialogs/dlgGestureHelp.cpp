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
  "- ![U](resource:IDB_GESTURE_UP) **Swipe Up** (U): Zoom in\n"
  "- ![D](resource:IDB_GESTURE_DOWN) **Swipe Down** (D): Zoom out\n"
  "- ![R](resource:IDB_GESTURE_RIGHT) **Swipe Right** (R): Previous screen mode\n"
  "- ![L](resource:IDB_GESTURE_LEFT) **Swipe Left** (L): Next screen mode\n"
  "- ![UD](resource:IDB_GESTURE_UD) **Up-Down** (UD): Auto zoom\n"
  "- ![DU](resource:IDB_GESTURE_DU) **Down-Up** (DU): Menu\n"
  "- ![DR](resource:IDB_GESTURE_DR) **Down-Right** (DR): Waypoint details\n"
  "- ![DL](resource:IDB_GESTURE_DL) **Down-Left** (DL): Alternates\n"
  "- ![RD](resource:IDB_GESTURE_RD) **Right-Down** (RD): Task manager\n"
  "- ![URD](resource:IDB_GESTURE_URD) **Up-Right-Down** (URD): Analysis\n"
  "- ![LDR](resource:IDB_GESTURE_LDR) **Left-Down-Right** (LDR): Checklist\n"
  "- ![URDL](resource:IDB_GESTURE_URDL) **Up-Right-Down-Left** (URDL): Pan mode\n"
  "- ![LDRDL](resource:IDB_GESTURE_LDRDL) **Left-Down-Right-Down-Left** (LDRDL): Status\n"
  "- ![ULDR](resource:IDB_GESTURE_ULDR) **Up-Left-Down-Right** (ULDR): Quick menu\n\n"
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
