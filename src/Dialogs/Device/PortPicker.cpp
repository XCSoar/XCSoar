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

#include "PortPicker.hpp"
#include "PortDataField.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Language/Language.hpp"

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#include "ScanBluetoothLeDialog.hpp"
#endif

bool
PortPicker(DataFieldEnum &df, const TCHAR *caption,
           const TCHAR *help_text) noexcept
{
  ComboList combo_list = df.CreateComboList(nullptr);

#ifdef ANDROID
  static constexpr int SCAN_BLUETOOTH_LE = -1;
  if (bluetooth_helper != nullptr &&
      bluetooth_helper->HasLe(Java::GetEnv()))
    combo_list.Append(SCAN_BLUETOOTH_LE, _("Bluetooth LE"));
#endif

  int i = ComboPicker(caption, combo_list, help_text);
  if (i < 0)
    return false;

  const ComboList::Item &item = combo_list[i];

#ifdef ANDROID
  if (item.int_value == SCAN_BLUETOOTH_LE) {
    auto [address, is_hm10] = ScanBluetoothLeDialog(*bluetooth_helper);
    if (address.empty())
        return false;

    const auto type = is_hm10
      ? DeviceConfig::PortType::BLE_HM10
      : DeviceConfig::PortType::BLE_SENSOR;

    SetBluetoothPort(df, type, address.c_str());
    return true;
  }
#endif

  df.SetFromCombo(item.int_value, item.string_value.c_str());
  return true;
}
