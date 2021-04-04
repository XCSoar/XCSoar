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

#include "ManageFlarmDialog.hpp"
#include "FLARM/ConfigWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Version.hpp"

class ManageFLARMWidget final
  : public RowFormWidget {
  enum Controls {
    Setup,
    Reboot,
  };

  FlarmDevice &device;
  const FlarmVersion version;

public:
  ManageFLARMWidget(const DialogLook &look, FlarmDevice &_device,
                    const FlarmVersion &version)
    :RowFormWidget(look), device(_device), version(version) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
ManageFLARMWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  if (version.available) {
    StaticString<64> buffer;

    if (!version.hardware_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.hardware_version.c_str());
      AddReadOnly(_("Hardware version"), NULL, buffer.c_str());
    }

    if (!version.software_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.software_version.c_str());
      AddReadOnly(_("Firmware version"), NULL, buffer.c_str());
    }

    if (!version.obstacle_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.obstacle_version.c_str());
      AddReadOnly(_("Obstacle database"), NULL, buffer.c_str());
    }
  }

  AddButton(_("Setup"), [this](){
    FLARMConfigWidget widget(GetLook(), device);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        _T("FLARM"), widget);
  });

  AddButton(_("Reboot"), [this](){
    MessageOperationEnvironment env;
    device.Restart(env);
  });
}

void
ManageFlarmDialog(Device &device, const FlarmVersion &version)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _T("FLARM"),
                      new ManageFLARMWidget(UIGlobals::GetDialogLook(),
                                            (FlarmDevice &)device, version));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
