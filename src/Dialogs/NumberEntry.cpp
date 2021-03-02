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

  TWidgetDialog<FixedWindowWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look, caption);

  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  /* create the input control */

  WindowStyle control_style;
  control_style.Hide();
  control_style.TabStop();

  auto entry = std::make_unique<DigitEntry>(look);
  entry->CreateSigned(client_area, client_area.GetClientRect(), control_style,
                      length, 0);
  entry->Resize(entry->GetRecommendedSize());
  entry->SetValue(value);

  /* create buttons */

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  /* run it */

  dialog.SetWidget(std::move(entry));

  bool result = dialog.ShowModal() == mrOK;
  if (!result)
    return false;

  value = ((DigitEntry &)dialog.GetWidget().GetWindow()).GetIntegerValue();
  return true;
}

bool
NumberEntryDialog(const TCHAR *caption,
                  unsigned &value, unsigned length)
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

  auto entry = std::make_unique<DigitEntry>(look);
  entry->CreateUnsigned(client_area, client_area.GetClientRect(), control_style,
                        length, 0);
  entry->Resize(entry->GetRecommendedSize());
  entry->SetValue(value);

  /* create buttons */

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  /* run it */

  dialog.SetWidget(std::move(entry));

  bool result = dialog.ShowModal() == mrOK;
  if (!result)
    return false;

  value = ((DigitEntry &)dialog.GetWidget().GetWindow()).GetUnsignedValue();
  return true;
}

bool
AngleEntryDialog(const TCHAR *caption, Angle &value)
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

  auto entry = std::make_unique<DigitEntry>(look);
  entry->CreateAngle(client_area, client_area.GetClientRect(), control_style);
  entry->Resize(entry->GetRecommendedSize());
  entry->SetValue(value);

  /* create buttons */

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  /* run it */

  dialog.SetWidget(std::move(entry));

  bool result = dialog.ShowModal() == mrOK;
  if (!result)
    return false;

  value = ((DigitEntry &)dialog.GetWidget().GetWindow()).GetAngleValue();
  return true;
}
