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

#include "Dialogs/NumberEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FixedWindowWidget.hpp"
#include "Form/DigitEntry.hpp"
#include "Language/Language.hpp"
#include "Math/Angle.hpp"
#include "UIGlobals.hpp"

bool
NumberEntryDialog(const TCHAR *caption,
                  int &value, unsigned length)
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
  entry.CreateSigned(client_area, client_area.GetClientRect(), control_style,
                     length, 0);
  entry.Resize(entry.GetRecommendedSize());
  entry.SetValue(value);

  /* create buttons */

  dialog.AddButton(_("OK"), dialog, mrOK);
  dialog.AddButton(_("Cancel"), dialog, mrCancel);

  /* run it */

  FixedWindowWidget widget(&entry);
  dialog.FinishPreliminary(&widget);

  bool result = dialog.ShowModal() == mrOK;
  dialog.StealWidget();
  if (!result)
    return false;

  value = entry.GetIntegerValue();
  return true;
}

bool
NumberEntryDialog(const TCHAR *caption,
                  unsigned &value, unsigned length)
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
  entry.CreateUnsigned(client_area, client_area.GetClientRect(), control_style,
                       length, 0);
  entry.Resize(entry.GetRecommendedSize());
  entry.SetValue(value);

  /* create buttons */

  dialog.AddButton(_("OK"), dialog, mrOK);
  dialog.AddButton(_("Cancel"), dialog, mrCancel);

  /* run it */

  FixedWindowWidget widget(&entry);
  dialog.FinishPreliminary(&widget);

  bool result = dialog.ShowModal() == mrOK;
  dialog.StealWidget();
  if (!result)
    return false;

  value = entry.GetUnsignedValue();
  return true;
}

bool
AngleEntryDialog(const TCHAR *caption, Angle &value)
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
  entry.CreateAngle(client_area, client_area.GetClientRect(), control_style);
  entry.Resize(entry.GetRecommendedSize());
  entry.SetValue(value);

  /* create buttons */

  dialog.AddButton(_("OK"), dialog, mrOK);
  dialog.AddButton(_("Cancel"), dialog, mrCancel);

  /* run it */

  FixedWindowWidget widget(&entry);
  dialog.FinishPreliminary(&widget);

  bool result = dialog.ShowModal() == mrOK;
  dialog.StealWidget();
  if (!result)
    return false;

  value = entry.GetAngleValue();
  return true;
}
