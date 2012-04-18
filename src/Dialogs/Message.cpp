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

#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "UIGlobals.hpp"

#include <assert.h>
#include <limits.h>

class ModalResultButton : public WndButton
{
  WndForm &form;
  int result;

public:
  ModalResultButton(ContainerWindow &parent, const DialogLook &look,
                    const TCHAR *caption,
                    const PixelRect &rc,
                    const WindowStyle style,
                    WndForm &_form, int _result)
    :WndButton(parent, look, caption, rc, style),
     form(_form), result(_result) {}

protected:
  virtual bool OnClicked() {
    form.SetModalResult(result);
    return true;
  }
};

int
ShowMessageBox(const TCHAR *text, const TCHAR *caption, unsigned flags)
{
  WndFrame *text_frame = NULL;
  WndButton *buttons[10];
  int button_count = 0;
  int i, res;

  assert(text != NULL);

  SingleWindow &main_window = UIGlobals::GetMainWindow();
  const PixelRect rc = main_window.GetClientRect();

  UPixelScalar dialog_width = Layout::Scale(200);
  UPixelScalar dialog_height = Layout::Scale(160);

  PixelScalar dialog_x = 0, dialog_y = 0;

  UPixelScalar button_width = Layout::Scale(60);
  UPixelScalar button_height = Layout::Scale(32);

  // Create dialog
  WindowStyle style;
  style.Hide();
  style.ControlParent();

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  PixelRect form_rc;
  form_rc.left = dialog_x;
  form_rc.top = dialog_y;
  form_rc.right = form_rc.left + dialog_width;
  form_rc.bottom = form_rc.top + dialog_height;

  WndForm wf(main_window, dialog_look, form_rc, caption, style);

  ContainerWindow &client_area = wf.GetClientAreaWindow();

  // Create text element
  text_frame = new WndFrame(client_area, dialog_look, 0, Layout::Scale(2),
                            dialog_width, dialog_height);

  text_frame->SetCaption(text);
  text_frame->SetAlignCenter();

  UPixelScalar text_height = text_frame->GetTextHeight();
  text_frame->Resize(dialog_width, text_height + Layout::Scale(2));

  dialog_height = wf.GetTitleHeight() + Layout::Scale(10) + text_height + button_height;
  dialog_x = ((rc.right - rc.left) - dialog_width) / 2;
  dialog_y = ((rc.bottom - rc.top) - dialog_height) / 2;
  wf.Move(dialog_x, dialog_y, dialog_width, dialog_height);

  PixelRect button_rc;
  button_rc.left = 0;
  button_rc.top = Layout::Scale(6) + text_height;
  button_rc.right = button_rc.left + button_width;
  button_rc.bottom = button_rc.top + button_height;

  // Create buttons
  WindowStyle button_style;
  button_style.TabStop();

  unsigned button_flags = flags & 0x000f;
  if (button_flags == MB_OK ||
      button_flags == MB_OKCANCEL) {
    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("OK"), button_rc,
                            button_style, wf, IDOK);

    button_count++;
  }

  if (button_flags == MB_YESNO ||
      button_flags == MB_YESNOCANCEL) {
    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("Yes"), button_rc,
                            button_style, wf, IDYES);

    button_count++;

    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("No"), button_rc,
                            button_style, wf, IDNO);

    button_count++;
  }

  if (button_flags == MB_ABORTRETRYIGNORE ||
      button_flags == MB_RETRYCANCEL) {
    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("Retry"), button_rc,
                            button_style, wf, IDRETRY);

    button_count++;
  }

  if (button_flags == MB_OKCANCEL ||
      button_flags == MB_RETRYCANCEL ||
      button_flags == MB_YESNOCANCEL) {
    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("Cancel"), button_rc,
                            button_style, wf, IDCANCEL);

    button_count++;
  }

  if (button_flags == MB_ABORTRETRYIGNORE) {
    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("Abort"), button_rc,
                            button_style, wf, IDABORT);

    button_count++;

    buttons[button_count] =
      new ModalResultButton(client_area, dialog_look, _("Ignore"), button_rc,
                            button_style, wf, IDIGNORE);

    button_count++;
  }

  UPixelScalar max_button_width = dialog_width / button_count;
  PixelScalar button_x = max_button_width / 2 - button_width / 2;

  // Move buttons to the right positions
  for (i = 0; i < button_count; i++) {
    buttons[i]->Move(button_x, button_rc.top);
    button_x += max_button_width;
  }

  // Show MessageBox and save result
  res = wf.ShowModal();

  delete text_frame;
  for (int i = 0; i < button_count; ++i)
    delete buttons[i];
  wf.reset();

  return res;
}
