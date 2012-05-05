/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/ManageFlarmDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/Driver/FLARM/Device.hpp"

class ManageFLARMWidget : public RowFormWidget, private ActionListener {
  enum Controls {
    Reboot,
  };

  FlarmDevice &device;

public:
  ManageFLARMWidget(const DialogLook &look, FlarmDevice &_device)
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id);
};

void
ManageFLARMWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddButton(_("Reboot"), this, Reboot);
}

void
ManageFLARMWidget::OnAction(int id)
{
  switch (id) {
  case Reboot:
    {
      MessageOperationEnvironment env;
      device.Restart(env);
    }
    break;
  }
}

void
ManageFlarmDialog(Device &device)
{
  WidgetDialog dialog(_T("FLARM"),
                      new ManageFLARMWidget(UIGlobals::GetDialogLook(),
                                            (FlarmDevice &)device));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
