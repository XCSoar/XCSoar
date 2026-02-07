// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/LockScreen.hpp"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/SingleWindow.hpp"
#include "UIGlobals.hpp"

#include <cassert>

void
ShowLockBox()
{
  auto &main_window = UIGlobals::GetMainWindow();

  const unsigned button_height = Layout::GetMinimumControlHeight();
  const unsigned button_width = button_height;

  WindowStyle style;
  style.ControlParent();

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  const PixelSize root_size = main_window.GetSize();
  
  // Position dialog where it shouldn't cover anything important on the screen
  const int dialog_x = root_size.width * 0.25 - button_width;
  const int dialog_y = root_size.height * 0.75 - button_height;

  PixelRect form_rc;
  form_rc.left = dialog_x;
  form_rc.top = dialog_y;
  form_rc.right = dialog_x + button_width;
  form_rc.bottom = dialog_y + button_height;

  WndForm wf(main_window, dialog_look, form_rc, NULL, style);

  ContainerWindow &client_area = wf.GetClientAreaWindow();
  
  const auto button_rc = client_area.GetClientRect();

  WindowStyle button_style;
  
  const Button button(client_area, dialog_look.button, "U", button_rc, button_style,
                      [&wf](){ wf.SetModalResult(mrCancel); });

  wf.ShowModal();
}
