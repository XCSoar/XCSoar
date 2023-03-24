// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DateEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FixedWindowWidget.hpp"
#include "Form/DigitEntry.hpp"
#include "Language/Language.hpp"
#include "time/BrokenDate.hpp"
#include "UIGlobals.hpp"

bool
DateEntryDialog(const TCHAR *caption, BrokenDate &value,
                bool nullable)
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
  entry->CreateDate(client_area, client_area.GetClientRect(), control_style);
  entry->Resize(entry->GetRecommendedSize());
  if (!value.IsPlausible())
    value = BrokenDate(1990, 1, 1);
  entry->SetValue(value);
  entry->SetCallback(dialog.MakeModalResultCallback(mrOK));

  /* create buttons */
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.AddButton(_("Reset"), [&entry = *entry, start_value = value](){
    entry.SetValue(start_value);  // the start value
  });

  if (nullable)
    dialog.AddButton(_("Clear"), [&entry=*entry](){
      entry.SetInvalid();
    });

  /* run it */

  dialog.SetWidget(std::move(entry));

  if (dialog.ShowModal() != mrOK)
    return false;

  value = ((DigitEntry &)dialog.GetWidget().GetWindow()).GetDateValue();
  return true;
}
