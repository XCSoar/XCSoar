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
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/ComboList.hpp"
#include "ui/event/Notify.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Android/UsbSerialHelper.hpp"
#include "Android/DetectDeviceListener.hpp"
#include "thread/Mutex.hxx"
#include <list>
#endif

#include <cassert>

class PortListItemRenderer final {
  TextRowRenderer row_renderer;

public:
  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void PaintItem(Canvas &canvas, PixelRect rc,
                 const ComboList::Item &item) noexcept {
    if (const TCHAR *text = ToDisplayString(DeviceConfig::PortType(item.int_value >> 16));
        text != nullptr)
      rc.right = row_renderer.DrawRightColumn(canvas, rc, text);

    row_renderer.DrawTextRow(canvas, rc, item.display_string.c_str());
  }

private:
  static const TCHAR *ToDisplayString(DeviceConfig::PortType type) noexcept {
    switch (type) {
    case DeviceConfig::PortType::RFCOMM:
      return _T("Bluetooth");

    case DeviceConfig::PortType::BLE_HM10:
      return _("BLE port");

    case DeviceConfig::PortType::BLE_SENSOR:
      return _("BLE sensor");

    case DeviceConfig::PortType::ANDROID_USB_SERIAL:
      return _("USB serial");

    default:
      return nullptr;
    }
  }
};

class PortPickerWidget
  : public ListWidget
#ifdef ANDROID
  , DetectDeviceListener
#endif
{
  WndForm &dialog;

  PortListItemRenderer item_renderer;

  DataFieldEnum &df;

  ComboList combo_list;

#ifdef ANDROID
  Java::LocalObject detect_listener;
  Java::LocalObject usb_serial_detect_listener;

  struct DetectedPort {
    DeviceConfig::PortType type;
    tstring address, name;
  };

  Mutex detected_mutex;
  std::list<DetectedPort> detected_list;

  UI::Notify detected_notify{[this]{ OnDetectedNotification(); }};
#endif

public:
  PortPickerWidget(WndForm &_dialog, DataFieldEnum &_df) noexcept
    :dialog(_dialog),
     df(_df) {}

private:
  const auto &GetSelectedItem() const noexcept {
    assert(!combo_list.empty());
    return combo_list[GetList().GetCursorIndex()];
  }

  void ReloadComboList() noexcept;

public:
  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    combo_list = df.CreateComboList(nullptr);

    const auto &look = dialog.GetLook();
    ListControl &list = CreateList(parent, look, rc,
                                   item_renderer.CalculateLayout(look));
    list.SetLength(combo_list.size());
    list.SetCursorIndex(combo_list.current_index);
  }

  bool Save(bool &changed) noexcept override {
    if (combo_list.empty())
      return true;

    const int old_value = df.GetAsInteger();
    const auto &item = GetSelectedItem();
    if (item.int_value == old_value)
      /* no change */
      return true;

    changed = true;
    df.SetFromCombo(item.int_value, item.string_value.c_str());
    return true;
  }

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);

#ifdef ANDROID
    if (bluetooth_helper != nullptr) {
      const auto env = Java::GetEnv();
      if (bluetooth_helper->HasLe(env))
        detect_listener =
          bluetooth_helper->AddDetectDeviceListener(env, *this);
    }

    if (usb_serial_helper != nullptr) {
      const auto env = Java::GetEnv();
      usb_serial_detect_listener =
        usb_serial_helper->AddDetectDeviceListener(env, *this);
    }
#endif
  }

  void Hide() noexcept override {
#ifdef ANDROID
    if (detect_listener) {
      bluetooth_helper->RemoveDetectDeviceListener(detect_listener.GetEnv(),
                                                   detect_listener);
      detect_listener = {};
    }

    if (usb_serial_detect_listener) {
      usb_serial_helper->RemoveDetectDeviceListener(usb_serial_detect_listener.GetEnv(),
                                                    usb_serial_detect_listener);
      usb_serial_detect_listener = {};
    }
#endif

    ListWidget::Hide();
  }

  /* virtual methods from class ListControl::Handler */

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    item_renderer.PaintItem(canvas, rc, combo_list[idx]);
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    dialog.SetModalResult(mrOK);
  }

#ifdef ANDROID
private:
  /* virtual methods from class DetectDeviceListener */
  void OnDeviceDetected(Type type, const char *address,
                        const char *name,
                        uint64_t features) noexcept override;

  void UpdateItem(DetectedPort &&detected) noexcept;
  void OnDetectedNotification() noexcept;
#endif
};

void
PortPickerWidget::ReloadComboList() noexcept
{
  const int old_value = combo_list.empty()
    ? -1
    : GetSelectedItem().int_value;

  combo_list = df.CreateComboList(nullptr);

  auto &list = GetList();
  list.SetLength(combo_list.size());

  int new_cursor = old_value >= 0 ? combo_list.Find(old_value) : -1;
  if (new_cursor >= 0)
    list.SetCursorIndex(new_cursor);

  list.Invalidate();
}

#ifdef ANDROID

void
PortPickerWidget::OnDeviceDetected(Type type, const char *address,
                                   const char *name,
                                   uint64_t features) noexcept
{
  if (name == nullptr)
    name = "";

  DeviceConfig::PortType port_type;
  switch (type) {
  case Type::IOIO:
    port_type = DeviceConfig::PortType::IOIOUART;
    break;

  case Type::BLUETOOTH_CLASSIC:
    port_type = DeviceConfig::PortType::RFCOMM;
    break;

  case Type::BLUETOOTH_LE:
    port_type = (features & DetectDeviceListener::FEATURE_HM10) != 0
      ? DeviceConfig::PortType::BLE_HM10
      : DeviceConfig::PortType::BLE_SENSOR;
    break;

  case Type::USB_SERIAL:
    port_type = DeviceConfig::PortType::ANDROID_USB_SERIAL;
    break;
  }

  {
    const std::lock_guard<Mutex> lock{detected_mutex};
    detected_list.emplace_back(DetectedPort{port_type, address, name});
  }

  detected_notify.SendNotification();
}

inline void
PortPickerWidget::UpdateItem(DetectedPort &&detected) noexcept
{
  UpdatePortEntry(df, detected.type, detected.address.c_str(),
                  detected.name.empty() ? nullptr : detected.name.c_str());
}

inline void
PortPickerWidget::OnDetectedNotification() noexcept
{
  {
    const std::lock_guard<Mutex> lock(detected_mutex);

    while (!detected_list.empty()) {
      UpdateItem(std::move(detected_list.front()));
      detected_list.pop_front();
    }
  }

  ReloadComboList();
}

#endif

bool
PortPicker(DataFieldEnum &df, const TCHAR *caption) noexcept
{
  TWidgetDialog<PortPickerWidget> dialog(WidgetDialog::Full{},
                                         UIGlobals::GetMainWindow(),
                                         UIGlobals::GetDialogLook(),
                                         caption);

  dialog.SetWidget(dialog, df);
  dialog.AddButton(_("Select"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();

  return dialog.GetChanged();
}
