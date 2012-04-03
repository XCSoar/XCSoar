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
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/Driver/FLARM/Device.hpp"

static WndForm *dialog;
static FlarmDevice *device;

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

static void
OnRebootClicked(gcc_unused WndButton &button)
{
  MessageOperationEnvironment env;
  device->Restart(env);
}

void
ManageFlarmDialog(Device &_device)
{
  device = (FlarmDevice *)&_device;

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.Hide();
  dialog_style.ControlParent();

  SingleWindow &parent = UIGlobals::GetMainWindow();
  const DialogLook &look = UIGlobals::GetDialogLook();

  dialog = new WndForm(parent, look, parent.get_client_rect(),
                       _T("FLARM"), dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  /* create buttons */

  ButtonPanel buttons(client_area, look);
  buttons.Add(_("Close"), OnCloseClicked);

  const PixelRect rc = buttons.UpdateLayout();

  /* create the command buttons */

  const UPixelScalar margin = 0;
  const UPixelScalar height = Layout::Scale(30);

  ButtonWindowStyle button_style;
  button_style.TabStop();

  WndButton *button;

  PixelRect brc = rc;
  brc.left += margin;
  brc.top += margin;
  brc.right -= margin;
  brc.bottom = brc.top + height;

  button = new WndButton(client_area, look, _("Reboot"),
                         brc,
                         button_style,
                         OnRebootClicked);
  dialog->AddDestruct(button);

  /* run it */

  dialog->ShowModal();

  delete dialog;
}
