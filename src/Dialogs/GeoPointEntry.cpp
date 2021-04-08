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

#include "Dialogs/GeoPointEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FixedWindowWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Form/DigitEntry.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Geo/GeoPoint.hpp"

bool
GeoPointEntryDialog(const TCHAR *caption, GeoPoint &value,
                    const CoordinateFormat format,
                    bool nullable)
{
  /* create the dialog */

  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<TwoWidgets> dialog(WidgetDialog::Auto{},
                                   UIGlobals::GetMainWindow(),
                                   look, caption);

  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  /* create the input control */

  WindowStyle control_style;
  control_style.Hide();
  control_style.TabStop();

  auto latitude_entry = std::make_unique<DigitEntry>(look);
  latitude_entry->CreateLatitude(client_area, client_area.GetClientRect(),
                                 control_style, format);
  latitude_entry->Resize(latitude_entry->GetRecommendedSize());
  latitude_entry->SetCallback(dialog.MakeModalResultCallback(mrOK));

  auto longitude_entry = std::make_unique<DigitEntry>(look);
  longitude_entry->CreateLongitude(client_area, client_area.GetClientRect(),
                                   control_style, format);
  longitude_entry->Resize(longitude_entry->GetRecommendedSize());
  longitude_entry->SetCallback(dialog.MakeModalResultCallback(mrOK));

  if (value.IsValid()) {
    latitude_entry->SetLatitude(value.latitude, format);
    longitude_entry->SetLongitude(value.longitude, format);
  } else {
    latitude_entry->SetInvalid();
    longitude_entry->SetInvalid();
  }

  /* create buttons */

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  if (nullable)
    dialog.AddButton(_("Clear"), [&latitude_entry=*latitude_entry, &longitude_entry=*longitude_entry](){
      latitude_entry.SetInvalid();
      longitude_entry.SetInvalid();
    });

  /* run it */

  dialog.SetWidget(std::make_unique<FixedWindowWidget>(std::move(latitude_entry)),
                   std::make_unique<FixedWindowWidget>(std::move(longitude_entry)),
                   true);

  if (dialog.ShowModal() != mrOK)
    return false;

  auto &la_entry = (DigitEntry &)((FixedWindowWidget &)dialog.GetWidget().GetFirst()).GetWindow();
  auto &lo_entry = (DigitEntry &)((FixedWindowWidget &)dialog.GetWidget().GetSecond()).GetWindow();

  value = GeoPoint(lo_entry.GetLongitude(format),
                   la_entry.GetLatitude(format));
  return true;
}
