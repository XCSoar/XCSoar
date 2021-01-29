/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
TimeEntryDialog(const TCHAR *caption, RoughTime &value,
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
