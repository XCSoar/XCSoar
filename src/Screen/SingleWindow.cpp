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

#include "Screen/SingleWindow.hpp"
#include "Form/Form.hpp"

void
SingleWindow::AddDialog(WndForm *dialog)
{
  dialogs.push_front(dialog);
}

void
SingleWindow::RemoveDialog(WndForm *dialog)
{
  assert(dialog == dialogs.front());

  dialogs.pop_front();
}

void
SingleWindow::CancelDialog()
{
  AssertThread();

  GetTopDialog().SetModalResult(mrCancel);
}

bool
SingleWindow::OnClose()
{
  if (!dialogs.empty()) {
    /* close the current dialog instead of the main window */
    CancelDialog();
    return true;
  }

  return TopWindow::OnClose();
}

void
SingleWindow::OnDestroy()
{
  TopWindow::OnDestroy();
  PostQuit();
}

void
SingleWindow::OnResize(PixelSize new_size)
{
  TopWindow::OnResize(new_size);

  const PixelRect rc = GetClientRect();
  for (WndForm *dialog : dialogs)
    dialog->ReinitialiseLayout(rc);
}
