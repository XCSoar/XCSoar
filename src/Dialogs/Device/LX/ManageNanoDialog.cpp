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

#include "ManageNanoDialog.hpp"
#include "NanoConfigWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "NMEA/DeviceInfo.hpp"

class ManageNanoWidget final
  : public RowFormWidget, private ActionListener {
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
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

void
ManageNanoWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
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

  AddButton(_("Setup"), *this, SETUP);
}

void
ManageNanoWidget::OnAction(int id)
{
  switch (id) {
  case SETUP:
    {
      NanoConfigWidget widget(GetLook(), device);
      DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                          _T("LXNAV Nano"), widget);
    }
    break;
  }
}

void
ManageNanoDialog(Device &device, const DeviceInfo &info)
{
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _T("LXNAV Nano"),
                    new ManageNanoWidget(UIGlobals::GetDialogLook(),
                                         (LXDevice &)device, info));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
