/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/ManageCAI302Dialog.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Language/Language.hpp"
#include "Operation.hpp"
#include "Device/Driver/CAI302/Internal.hpp"

static WndForm *dialog;
static CAI302Device *device;

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

static void
OnStartLogging(gcc_unused WndButton &button)
{
  MessageOperationEnvironment env;
  device->StartLogging(env);
}

static void
OnStopLogging(gcc_unused WndButton &button)
{
  MessageOperationEnvironment env;
  device->StopLogging(env);
}

static void
OnClearLogClicked(gcc_unused WndButton &button)
{
  if (MessageBoxX(_("Do you really want to delete all flights from the device?"),
                  _T("CAI 302"), MB_YESNO) != IDYES)
    return;

  MessageOperationEnvironment env;
  device->ClearLog(env);
}

static void
OnRebootClicked(gcc_unused WndButton &button)
{
  MessageOperationEnvironment env;
  device->Reboot(env);
}

void
ManageCAI302Dialog(SingleWindow &parent, const DialogLook &look,
                   Device &_device)
{
  device = (CAI302Device *)&_device;

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.hide();
  dialog_style.control_parent();

  PixelSize size = parent.get_size();
  dialog = new WndForm(parent, look, 0, 0, size.cx, size.cy,
                       _T("CAI 302"), dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  /* create buttons */

  ButtonPanel buttons(client_area, look);
  const PixelRect rc = buttons.GetRemainingRect();

  buttons.Add(_("Close"), OnCloseClicked);

  /* create the command buttons */

  const unsigned margin = Layout::Scale(2);
  const unsigned height = Layout::Scale(32);

  ButtonWindowStyle button_style;
  button_style.tab_stop();

  unsigned y = rc.top + margin;
  WndButton *button;

  button = new WndButton(client_area, look, _("Start Logger"),
                         rc.left + margin, y,
                         rc.right - rc.left - 2 * margin, height,
                         button_style,
                         OnStartLogging);
  dialog->AddDestruct(button);
  y += height + margin;

  button = new WndButton(client_area, look, _("Stop Logger"),
                         rc.left + margin, y,
                         rc.right - rc.left - 2 * margin, height,
                         button_style,
                         OnStopLogging);
  dialog->AddDestruct(button);
  y += height + margin;

  button = new WndButton(client_area, look, _("Delete all flights"),
                         rc.left + margin, y,
                         rc.right - rc.left - 2 * margin, height,
                         button_style,
                         OnClearLogClicked);
  dialog->AddDestruct(button);
  y += height + margin;

  button = new WndButton(client_area, look, _("Reboot"),
                         rc.left + margin, y,
                         rc.right - rc.left - 2 * margin, height,
                         button_style,
                         OnRebootClicked);
  dialog->AddDestruct(button);
  y += height + margin;

  /* run it */

  dialog->ShowModal();

  delete dialog;
}
