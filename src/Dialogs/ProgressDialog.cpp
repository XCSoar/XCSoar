/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Screen/ButtonWindow.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"

#include <assert.h>

class ProgressCancelButton : public ButtonWindow {
   std::function<void()> callback;

public:
  ProgressCancelButton(std::function<void()> &&_callback)
    :callback(std::move(_callback)) {}

  virtual bool OnClicked() {
    callback();
    return true;
  }
};

ProgressDialog::ProgressDialog(SingleWindow &parent,
                               const DialogLook &dialog_look,
                               const TCHAR *caption)
  :WndForm(parent, dialog_look, parent.GetClientRect(), caption),
   progress(GetClientAreaWindow()),
   cancel_button(nullptr)
{
}

ProgressDialog::~ProgressDialog()
{
  delete cancel_button;
}

void
ProgressDialog::AddCancelButton(std::function<void()> &&callback)
{
  assert(cancel_button == nullptr);

  ButtonWindowStyle style;
  style.TabStop();

  PixelRect rc = client_area.GetClientRect();
  rc.right -= Layout::Scale(2);
  rc.left = rc.right - Layout::Scale(78);
  rc.top += Layout::Scale(2);
  rc.bottom = rc.top + Layout::Scale(35);

  cancel_button = new ProgressCancelButton(std::move(callback));
  cancel_button->Create(client_area, _("Cancel"), rc, style);
  cancel_button->SetFont(*GetLook().button.font);
  cancel_button->BringToTop();
}
