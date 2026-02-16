// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/TimeEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FixedWindowWidget.hpp"
#include "Form/DigitEntry.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "time/RoughTime.hpp"
#include "time/BrokenDateTime.hpp"

enum {
  CLEAR = 100,
};

bool
TimeEntryDialog(const char *caption, RoughTime &value,
                RoughTimeDelta time_zone, bool nullable)
{
  /* create the dialog */

  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<FixedWindowWidget> dialog(WidgetDialog::Auto{},
                                          UIGlobals::GetMainWindow(),
                                          look, caption);

  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  /* create the input control */

  WindowStyle control_style;
  control_style.Hide();
  control_style.TabStop();

  auto entry = std::make_unique<DigitEntry>(look);
  entry->CreateTime(client_area, client_area.GetClientRect(), control_style);
  entry->Resize(entry->GetRecommendedSize());
  entry->SetValue(value + time_zone);
  entry->SetCallback(dialog.MakeModalResultCallback(mrOK));

  /* create buttons */

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.AddButton(_("Now"), [&entry = *entry, time_zone](){
    const BrokenTime bt = BrokenDateTime::NowUTC();
    RoughTime now_utc = RoughTime(bt.hour, bt.minute);
    entry.SetValue(now_utc + time_zone);
  });

  if (nullable)
    dialog.AddButton(_("Clear"), [&entry=*entry](){
      entry.SetInvalid();
    });

  /* run it */

  dialog.SetWidget(std::move(entry));

  if (dialog.ShowModal() != mrOK)
    return false;

  value = ((DigitEntry &)dialog.GetWidget().GetWindow()).GetTimeValue() - time_zone;
  return true;
}
