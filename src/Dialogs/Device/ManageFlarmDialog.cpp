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
  : public RowFormWidget, private ActionListener {
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
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

void
ManageFLARMWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
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

  AddButton(_("Setup"), *this, Setup);
  AddButton(_("Reboot"), *this, Reboot);
}

void
ManageFLARMWidget::OnAction(int id)
{
  switch (id) {
  case Setup:
    {
      FLARMConfigWidget widget(GetLook(), device);
      DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                          _T("FLARM"), widget);
    }
    break;

  case Reboot:
    {
      MessageOperationEnvironment env;
      device.Restart(env);
    }
    break;
  }
}

void
ManageFlarmDialog(Device &device, const FlarmVersion &version)
{
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _T("FLARM"),
                    new ManageFLARMWidget(UIGlobals::GetDialogLook(),
                                          (FlarmDevice &)device, version));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
