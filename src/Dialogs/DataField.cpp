// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataField.hpp"
#include "FilePicker.hpp"
#include "Form/DataField/GeoPoint.hpp"
#include "Form/DataField/RoughTime.hpp"
#include "Form/DataField/Prefix.hpp"
#include "Form/DataField/Date.hpp"
#include "Form/DataField/Integer.hpp"
#include "Form/DataField/Frequency.hpp"
#include "ComboPicker.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/TimeEntry.hpp"
#include "Dialogs/GeoPointEntry.hpp"
#include "Dialogs/DateEntry.hpp"
#include "Dialogs/NumberEntry.hpp"
#include "Dialogs/RadioFrequencyEntry.hpp"

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/TextEntryDialog.hpp"
#endif

#include <algorithm>

bool
EditDataFieldDialog(const TCHAR *caption, DataField &df,
                    const TCHAR *help_text)
{
  const auto type = df.GetType();
  if (type == DataField::Type::FILE) {
    return FilePicker(caption, (FileDataField &)df, help_text);
  } else if (df.SupportsCombolist()) {
    return ComboPicker(caption, df, help_text);
  } else if (type == DataField::Type::ROUGH_TIME) {
    RoughTimeDataField &tdf = (RoughTimeDataField &)df;
    RoughTime value = tdf.GetValue();
    if (!TimeEntryDialog(caption, value, tdf.GetTimeZone(), true))
      return false;

    tdf.ModifyValue(value);
    return true;
  } else if (type == DataField::Type::GEOPOINT) {
    GeoPointDataField &gdf = (GeoPointDataField &)df;
    GeoPoint value = gdf.GetValue();
    if (!GeoPointEntryDialog(caption, value,
                             gdf.GetFormat(),
                             false))
      return false;

    gdf.ModifyValue(value);
    return true;
  } else if (type == DataField::Type::DATE) {
    auto &dfd = static_cast<DataFieldDate &>(df);
    BrokenDate date = dfd.GetValue();
    if (!DateEntryDialog(caption, date, true))
      return false;

    dfd.SetValue(date);
    return true;
  } else if (type == DataField::Type::INTEGER) {
    auto &dfi = static_cast<DataFieldInteger &>(df);

    // signed or unsigned depends on min if value >= 0 or < 0...
    if (dfi.GetMin() >= 0) {
      unsigned value = dfi.GetValue(); // min is >= 0!
      if (!NumberEntryDialog(caption, value,
          log10(dfi.GetMax()) + 1))
        return false;

      dfi.ModifyValue(value); // SetAsInteger with unsigned!
      return true;
    } else {
      /* with signed range has to avoid the length of negative AND
      * positiv numbers */
      int value = dfi.GetValue();  // min is < 0!
      unsigned max = std::max(abs(dfi.GetMax()), abs(dfi.GetMin()));
      if (!NumberEntryDialog(caption, value, log10(max) + 1))
        return false;

      dfi.ModifyValue(value);  // SetAsInteger with signed!
      return true;
    }
  } else if (type == DataField::Type::STRING ||
             type == DataField::Type::PREFIX) {
    auto &sdf = static_cast<DataFieldString &>(df);

    const TCHAR *value = sdf.GetValue();
    assert(value != nullptr);

    StaticString<EDITSTRINGSIZE> buffer(value);

    PrefixDataField::AllowedCharactersFunction acf;
    if (type == DataField::Type::PREFIX)
      acf = static_cast<PrefixDataField &>(sdf).GetAllowedCharactersFunction();

#ifdef ANDROID
    if (!acf) {
      /* not using AndroidTextEntryDialog::Type::PASSWORD for
         PasswordDataField because AndroidTextEntryDialog doesn't have
         an option (yet) to reveal the password */
      auto type = AndroidTextEntryDialog::Type::TEXT;

      AndroidTextEntryDialog dlg;
      auto new_value = dlg.ShowModal(Java::GetEnv(), *context,
                                     caption, value, type);
      if (!new_value)
        return false;

      sdf.ModifyValue(new_value->c_str());
      return true;
    }
#endif

    if (!TextEntryDialog(buffer, caption, acf))
      return false;

    sdf.ModifyValue(buffer);
    return true;
  } else if (type == DataField::Type::RADIO_FREQUENCY) {
    RadioFrequencyDataField &fdf = (RadioFrequencyDataField &)df;
    RadioFrequency value = fdf.GetValue();
    if (!RadioFrequencyEntryDialog(caption, value, false))
      return false;

    fdf.ModifyValue(value);
    return true;
  } else
    // don't know how to edit this
    return false;
}
