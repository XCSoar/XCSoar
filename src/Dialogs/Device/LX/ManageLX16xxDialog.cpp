// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageLX16xxDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "NMEA/DeviceInfo.hpp"

class ManageLX16xxWidget : public RowFormWidget {
  /* This attribute is will be used in the next commits. */
  gcc_unused_field LXDevice &device;
  const DeviceInfo info;

public:
  ManageLX16xxWidget(const DialogLook &look, LXDevice &_device,
                     const DeviceInfo &info)
    :RowFormWidget(look), device(_device), info(info) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept;
};

void
ManageLX16xxWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  StaticString<64> buffer;

  if (!info.product.empty()) {
    buffer.SetASCII(info.product.c_str());
    AddReadOnly(_("Product"), NULL, buffer.c_str());
  }

  if (!info.serial.empty()) {
    buffer.SetASCII(info.serial.c_str());
    AddReadOnly(_("Serial"), NULL, buffer.c_str());
  }

  if (!info.hardware_version.empty()) {
    buffer.SetASCII(info.hardware_version.c_str());
    AddReadOnly(_("Hardware version"), NULL, buffer.c_str());
  }

  if (!info.software_version.empty()) {
    buffer.SetASCII(info.software_version.c_str());
    AddReadOnly(_("Firmware version"), NULL, buffer.c_str());
  }
}

void
ManageLX16xxDialog(Device &device, const DeviceInfo &info)
{
  StaticString<64> title;
  title.Format("LX %s", info.product.c_str());

  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      title,
                      new ManageLX16xxWidget(UIGlobals::GetDialogLook(),
                                             (LXDevice &)device, info));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
