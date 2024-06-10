// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadioFrequencyEntry.hpp"
#include "Dialogs/GeoPointEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FixedWindowWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Form/DigitEntry.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "RadioFrequency.hpp"

bool RadioFrequencyEntryDialog(const TCHAR *caption,
                               RadioFrequency &value,
                               bool nullable)
{
  /* create the dialog */

  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<FixedWindowWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look, caption);

  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  /* create the input control */

  WindowStyle control_style;
  control_style.Hide();
  control_style.TabStop();

  auto frequency_entry = std::make_unique<DigitEntry>(look);
  frequency_entry->CreateRadioFrequency(client_area, client_area.GetClientRect(),
                                 control_style);
  frequency_entry->Resize(frequency_entry->GetRecommendedSize());
  frequency_entry->SetCallback(dialog.MakeModalResultCallback(mrOK));

  if (value.IsDefined())
    frequency_entry->SetValue(value);
  else
    frequency_entry->SetInvalid();

  /* create buttons */

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  if (nullable)
    dialog.AddButton(_("Clear"), [&frequency_entry = *frequency_entry]()
                     {
      frequency_entry.SetInvalid();});

  /* run it */

  dialog.SetWidget(std::move(frequency_entry));

  if (dialog.ShowModal() != mrOK)
    return false;

  auto &entry = ((DigitEntry &)dialog.GetWidget().GetWindow());
  value = RadioFrequency(entry.GetRadioFrequency());
  
  return true;
}
