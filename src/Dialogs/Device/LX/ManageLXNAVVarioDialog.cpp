// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageLXNAVVarioDialog.hpp"
#include "LXNAVVarioConfigWidget.hpp"
#include "ManageNanoDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "NMEA/DeviceInfo.hpp"

class ManageLXNAVVarioWidget final
  : public RowFormWidget {
  LXDevice &device;
  const DeviceInfo info;
  const DeviceInfo secondary_info;

public:
  ManageLXNAVVarioWidget(const DialogLook &look, LXDevice &_device,
                 const DeviceInfo &info,
                 const DeviceInfo &secondary_info)
    :RowFormWidget(look), device(_device), info(info),
     secondary_info(secondary_info) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
ManageLXNAVVarioWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  StaticString<64> buffer;

  if (!info.product.empty()) {
    buffer.clear();
    buffer.UnsafeAppendASCII(info.product.c_str());
    AddReadOnly(_("Product"), NULL, buffer.c_str());
  }

  if (!info.serial.empty()) {
    buffer.clear();
    buffer.UnsafeAppendASCII(info.serial.c_str());
    AddReadOnly(_("Serial"), NULL, buffer.c_str());
  }

  if (!info.hardware_version.empty()) {
    buffer.clear();
    buffer.UnsafeAppendASCII(info.hardware_version.c_str());
    AddReadOnly(_("Hardware version"), NULL, buffer.c_str());
  }

  if (!info.software_version.empty()) {
    buffer.clear();
    buffer.UnsafeAppendASCII(info.software_version.c_str());
    AddReadOnly(_("Firmware version"), NULL, buffer.c_str());
  }

  AddButton(_("Setup"), [this](){
    LXNAVVarioConfigWidget widget(GetLook(), device);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        "LXNAV Vario", widget);
  });

  if (device.IsNano())
    AddButton("LXNAV Nano", [this](){
      MessageOperationEnvironment env;
      if (device.EnablePassThrough(env)) {
        ManageNanoDialog(device, secondary_info);
        device.EnableNMEA(env);
      }
    });
}

void
ManageLXNAVVarioDialog(Device &device, const DeviceInfo &info,
               const DeviceInfo &secondary_info)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      "LXNAV Vario",
                      new ManageLXNAVVarioWidget(UIGlobals::GetDialogLook(),
                                         (LXDevice &)device, info,
                                         secondary_info));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
