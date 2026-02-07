// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageNanoDialog.hpp"
#include "NanoConfigWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "NMEA/DeviceInfo.hpp"

class ManageNanoWidget final
  : public RowFormWidget {
  enum Controls {
    SETUP,
  };

  LXDevice &device;
  const DeviceInfo info;

public:
  ManageNanoWidget(const DialogLook &look, LXDevice &_device,
                 const DeviceInfo &info)
    :RowFormWidget(look), device(_device), info(info) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
ManageNanoWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
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
    NanoConfigWidget widget(GetLook(), device);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        "LXNAV Nano", widget);
  });
}

void
ManageNanoDialog(Device &device, const DeviceInfo &info)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      "LXNAV Nano",
                      new ManageNanoWidget(UIGlobals::GetDialogLook(),
                                           (LXDevice &)device, info));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
