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

#include "Dialogs/LockScreen.hpp"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "UIGlobals.hpp"

#include <assert.h>

void
ShowLockBox()
{
  SingleWindow &main_window = UIGlobals::GetMainWindow();

  const unsigned button_height = Layout::GetMinimumControlHeight();
  const unsigned button_width = button_height;

  WindowStyle style;
  style.ControlParent();

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  const PixelSize root_size = main_window.GetSize();
  
  // Position dialog where it shouldn't cover anything important on the screen
  const int dialog_x = root_size.cx * 0.25 - button_width;
  const int dialog_y = root_size.cy * 0.75 - button_height;

  PixelRect form_rc;
  form_rc.left = dialog_x;
  form_rc.top = dialog_y;
  form_rc.right = dialog_x + button_width;
  form_rc.bottom = dialog_y + button_height;

  WndForm wf(main_window, dialog_look, form_rc, NULL, style);

  ContainerWindow &client_area = wf.GetClientAreaWindow();
  
  const auto button_rc = client_area.GetClientRect();

  WindowStyle button_style;
  
  const Button button(client_area, dialog_look.button, _T("U"), button_rc, button_style, wf, 3);

  wf.ShowModal();
}
