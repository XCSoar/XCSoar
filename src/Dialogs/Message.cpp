/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/SingleWindow.hpp"
#include "UIGlobals.hpp"

#include <boost/container/static_vector.hpp>

#include <cassert>

int
ShowMessageBox(const TCHAR *text, const TCHAR *caption,
               unsigned flags) noexcept
{
  assert(text != NULL);

  auto &main_window = UIGlobals::GetMainWindow();
  const auto main_rc = main_window.GetClientRect();

  PixelSize client_area_size(Layout::Scale(200u), Layout::Scale(160u));

  const auto button_size = Layout::Scale(PixelSize{60u, 32u});

  // Create dialog
  WindowStyle style;
  style.Hide();
  style.ControlParent();

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  WndForm wf(main_window, dialog_look, PixelRect{client_area_size},
             caption, style);

  ContainerWindow &client_area = wf.GetClientAreaWindow();

  // Create text element
  WndFrame text_frame(client_area, dialog_look,
                      client_area.GetClientRect());

  text_frame.SetText(text);
  text_frame.SetAlignCenter();

  const unsigned text_height = text_frame.GetTextHeight();
  text_frame.Resize(client_area_size.width,
                    text_height + Layout::GetTextPadding());

  client_area_size.height = Layout::Scale(10) + text_height + button_size.height;

  const auto dialog_size = wf.ClientAreaToDialogSize(client_area_size);
  const auto dialog_position = main_rc.CenteredTopLeft(dialog_size);

  const PixelRect dialog_rc(dialog_position, dialog_size);
  wf.Move(dialog_rc);

  const PixelRect button_rc(PixelPoint(0, Layout::Scale(6u) + text_height),
                            button_size);

  // Create buttons
  WindowStyle button_style;
  button_style.TabStop();

  boost::container::static_vector<Button, 10> buttons;

  unsigned button_flags = flags & 0x000f;
  if (button_flags == MB_OK ||
      button_flags == MB_OKCANCEL)
    buttons.emplace_back(client_area, dialog_look.button, _("OK"), button_rc,
                         button_style,
                         [&wf](){ wf.SetModalResult(IDOK); });

  if (button_flags == MB_YESNO ||
      button_flags == MB_YESNOCANCEL) {
    buttons.emplace_back(client_area, dialog_look.button, _("Yes"), button_rc,
                         button_style, wf.MakeModalResultCallback(IDYES));
    buttons.emplace_back(client_area, dialog_look.button, _("No"), button_rc,
                         button_style, wf.MakeModalResultCallback(IDNO));
  }

  if (button_flags == MB_ABORTRETRYIGNORE ||
      button_flags == MB_RETRYCANCEL)
    buttons.emplace_back(client_area, dialog_look.button, _("Retry"), button_rc,
                         button_style, wf.MakeModalResultCallback(IDRETRY));

  if (button_flags == MB_OKCANCEL ||
      button_flags == MB_RETRYCANCEL ||
      button_flags == MB_YESNOCANCEL)
    buttons.emplace_back(client_area, dialog_look.button, _("Cancel"), button_rc,
                         button_style, wf.MakeModalResultCallback(IDCANCEL));

  if (button_flags == MB_ABORTRETRYIGNORE) {
    buttons.emplace_back(client_area, dialog_look.button, _("Abort"), button_rc,
                         button_style, wf.MakeModalResultCallback(IDABORT));

    buttons.emplace_back(client_area, dialog_look.button, _("Ignore"), button_rc,
                         button_style, wf.MakeModalResultCallback(IDIGNORE));
  }

  const unsigned max_button_width = client_area_size.width / buttons.size();
  int button_x = max_button_width / 2 - button_size.width / 2;

  // Move buttons to the right positions
  for (unsigned i = 0; i < buttons.size(); i++) {
    buttons[i].Move(button_x, button_rc.top);
    button_x += max_button_width;
  }

  // Show MessageBox and save result
  unsigned res = wf.ShowModal();

  return res;
}
