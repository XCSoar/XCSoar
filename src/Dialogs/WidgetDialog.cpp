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

#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/Widget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

static Widget *widget;
static WndForm *dialog;
static bool changed;

static void
OnOKClicked(gcc_unused WndButton &button)
{
  bool require_restart;
  if (widget->Save(changed, require_restart))
    dialog->SetModalResult(mrOK);
}

static void
OnCancelClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrCancel);
}

bool
WidgetDialog(const TCHAR *caption, const PixelRect &rc, Widget &_widget)
{
  widget = &_widget;

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.Hide();
  dialog_style.ControlParent();

  dialog = new WndForm(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                       rc,caption, dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  ButtonPanel buttons(client_area, UIGlobals::GetDialogLook());
  buttons.Add(_("OK"), OnOKClicked);
  buttons.Add(_("Cancel"), OnCancelClicked);

  const PixelRect remaining_rc = buttons.GetRemainingRect();

  widget->Initialise(client_area, remaining_rc);
  widget->Prepare(client_area, remaining_rc);
  widget->Show(remaining_rc);

  changed = false;
  dialog->ShowModal();

  widget->Hide();
  widget->Unprepare();
  delete dialog;

  return changed;
}
