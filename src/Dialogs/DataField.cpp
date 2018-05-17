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

#include "DataField.hpp"
#include "FilePicker.hpp"
#include "Form/DataField/GeoPoint.hpp"
#include "Form/DataField/RoughTime.hpp"
#include "Form/DataField/Prefix.hpp"
#include "ComboPicker.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/TimeEntry.hpp"
#include "Dialogs/GeoPointEntry.hpp"

bool
EditDataFieldDialog(const TCHAR *caption, DataField &df,
                    const TCHAR *help_text)
{
  if (df.GetType() == DataField::Type::FILE) {
    return FilePicker(caption, (FileDataField &)df, help_text);
  } else if (df.SupportsCombolist()) {
    return ComboPicker(caption, df, help_text);
  } else if (df.GetType() == DataField::Type::ROUGH_TIME) {
    RoughTimeDataField &tdf = (RoughTimeDataField &)df;
    RoughTime value = tdf.GetValue();
    if (!TimeEntryDialog(caption, value, tdf.GetTimeZone(), true))
      return false;

    tdf.ModifyValue(value);
    return true;
  } else if (df.GetType() == DataField::Type::GEOPOINT) {
    GeoPointDataField &gdf = (GeoPointDataField &)df;
    GeoPoint value = gdf.GetValue();
    if (!GeoPointEntryDialog(caption, value,
                             gdf.GetFormat(),
                             false))
      return false;

    gdf.ModifyValue(value);
    return true;
  } else {
    const TCHAR *value = df.GetAsString();
    if (value == NULL)
      return false;

    StaticString<EDITSTRINGSIZE> buffer(value);

    PrefixDataField::AllowedCharactersFunction acf;
    if (df.GetType() == DataField::Type::PREFIX)
      acf = ((PrefixDataField &)df).GetAllowedCharactersFunction();

    if (!TextEntryDialog(buffer, caption, acf))
      return false;

    df.SetAsString(buffer);
    return true;
  }
}
