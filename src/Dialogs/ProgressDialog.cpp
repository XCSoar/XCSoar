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

#include "ProgressDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"

#include <assert.h>

ProgressDialog::ProgressDialog(SingleWindow &parent,
                               const DialogLook &dialog_look,
                               const TCHAR *caption)
  :WndForm(parent, dialog_look, parent.GetClientRect(), caption),
   progress(GetClientAreaWindow())
{
}

void
ProgressDialog::AddCancelButton(std::function<void()> &&callback)
{
  assert(!cancel_button.IsDefined());

  WindowStyle style;
  style.TabStop();

  PixelRect rc = client_area.GetClientRect();
  rc.right -= Layout::Scale(2);
  rc.left = rc.right - Layout::Scale(78);
  rc.top += Layout::Scale(2);
  rc.bottom = rc.top + Layout::Scale(35);

  cancel_button.Create(client_area, GetLook().button,
                       _("Cancel"), rc, style,
                       *this, mrCancel);
  cancel_button.BringToTop();

  cancel_callback = std::move(callback);
}

void
ProgressDialog::OnAction(int id)
{
  if (id == mrCancel && cancel_callback)
    cancel_callback();
  else
    WndForm::OnAction(id);
}
