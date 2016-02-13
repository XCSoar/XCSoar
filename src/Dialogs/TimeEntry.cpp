/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Form/LambdaActionListener.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Time/RoughTime.hpp"
#include "Time/BrokenDateTime.hpp"

enum {
  CLEAR = 100,
};

bool
TimeEntryDialog(const TCHAR *caption, RoughTime &value,
                RoughTimeDelta time_zone, bool nullable)
{
  /* create the dialog */

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(look);
  dialog.CreatePreliminary(UIGlobals::GetMainWindow(), caption);

  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  /* create the input control */

  WindowStyle control_style;
  control_style.Hide();
  control_style.TabStop();

  DigitEntry entry(look);
  entry.CreateTime(client_area, client_area.GetClientRect(), control_style);
  entry.Resize(entry.GetRecommendedSize());
  entry.SetValue(value + time_zone);
  entry.SetActionListener(dialog, mrOK);

  /* create buttons */

  dialog.AddButton(_("OK"), dialog, mrOK);
  dialog.AddButton(_("Cancel"), dialog, mrCancel);

  auto now_listener = MakeLambdaActionListener([&entry, time_zone](unsigned){
      const BrokenTime bt = BrokenDateTime::NowUTC();
      RoughTime now_utc = RoughTime(bt.hour, bt.minute);
      entry.SetValue(now_utc + time_zone);
    });
  dialog.AddButton(_("Now"), now_listener, 0);

  auto clear_listener = MakeLambdaActionListener([&entry](unsigned){
      entry.SetInvalid();
    });
  if (nullable)
    dialog.AddButton(_("Clear"), clear_listener, 0);

  /* run it */

  FixedWindowWidget widget(&entry);
  dialog.FinishPreliminary(&widget);

  bool result = dialog.ShowModal() == mrOK;
  dialog.StealWidget();
  if (!result)
    return false;

  value = entry.GetTimeValue() - time_zone;
  return true;
}
