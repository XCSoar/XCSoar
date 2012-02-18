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

class ModalResultButton : public WndButton {
  WndForm &form;
  int result;

public:
  ModalResultButton(ContainerWindow &parent, const DialogLook &look,
                    const TCHAR *Caption,
                    const PixelRect &rc,
                    const WindowStyle style,
                    WndForm &_form, int _result)
    :WndButton(parent, look, Caption, rc,
               style),
     form(_form), result(_result) {}

protected:
  virtual bool OnClicked() {
    form.SetModalResult(result);
    return true;
  }
};

// Message Box Replacement
/**
 * Displays a MessageBox and returns the pressed button
 * @param lpText Text displayed inside the MessageBox
 * @param lpCaption Text displayed in the Caption of the MessageBox
 * @param uType Type of MessageBox to display (OK+Cancel, Yes+No, etc.)
 * @return
 */
int
MessageBoxX(const TCHAR *lpText, const TCHAR *lpCaption, unsigned uType)
{
  WndFrame *wText = NULL;
  WndButton *wButtons[10];
  int ButtonCount = 0;
  int i, res;

  assert(lpText != NULL);

  SingleWindow &main_window = UIGlobals::GetMainWindow();
  const PixelRect rc = main_window.get_client_rect();

  UPixelScalar Width = Layout::Scale(200);
  UPixelScalar Height = Layout::Scale(160);

  PixelScalar X = 0, Y = 0;

  UPixelScalar w = Layout::Scale(60);
  UPixelScalar h = Layout::Scale(32);

  // Create dialog
  WindowStyle style;
  style.Hide();
  style.ControlParent();

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();
  PixelRect form_rc;
  form_rc.left = X;
  form_rc.top = Y;
  form_rc.right = form_rc.left + Width;
  form_rc.bottom = form_rc.top + Height;
  WndForm wf(main_window, dialog_look,
             form_rc, lpCaption,
             style);
  ContainerWindow &client_area = wf.GetClientAreaWindow();

  // Create text element
  wText = new WndFrame(client_area, dialog_look,
                       0, Layout::Scale(2), Width, Height);

  wText->SetCaption(lpText);
  wText->SetAlignCenter();

  UPixelScalar text_height = wText->GetTextHeight();
  wText->resize(Width, text_height + Layout::Scale(2));

  Height = wf.GetTitleHeight() + Layout::Scale(10) + text_height + h;
  X = ((rc.right - rc.left) - Width) / 2;
  Y = ((rc.bottom - rc.top) - Height) / 2;
  wf.move(X, Y, Width, Height);

  PixelRect button_rc;
  button_rc.left = 0;
  button_rc.top = Layout::Scale(6) + text_height;
  button_rc.right = button_rc.left + w;
  button_rc.bottom = button_rc.top + h;

  // Create buttons
  WindowStyle button_style;
  button_style.TabStop();

  uType = uType & 0x000f;
  if (uType == MB_OK || uType == MB_OKCANCEL) {
    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("OK"), button_rc,
                            button_style, wf, IDOK);

    ButtonCount++;
  }

  if (uType == MB_YESNO || uType == MB_YESNOCANCEL) {
    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("Yes"), button_rc,
                            button_style, wf, IDYES);

    ButtonCount++;

    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("No"), button_rc,
                            button_style, wf, IDNO);

    ButtonCount++;
  }

  if (uType == MB_ABORTRETRYIGNORE || uType == MB_RETRYCANCEL) {
    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("Retry"), button_rc,
                            button_style, wf, IDRETRY);

    ButtonCount++;
  }

  if (uType == MB_OKCANCEL || uType == MB_RETRYCANCEL || uType == MB_YESNOCANCEL) {
    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("Cancel"), button_rc,
                            button_style, wf, IDCANCEL);

    ButtonCount++;
  }

  if (uType == MB_ABORTRETRYIGNORE) {
    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("Abort"), button_rc,
                            button_style, wf, IDABORT);

    ButtonCount++;

    wButtons[ButtonCount] =
      new ModalResultButton(client_area, dialog_look, _("Ignore"), button_rc,
                            button_style, wf, IDIGNORE);

    ButtonCount++;
  }

  UPixelScalar d = Width / (ButtonCount);
  PixelScalar x = d / 2 - w / 2;

  // Move buttons to the right positions
  for (i = 0; i < ButtonCount; i++) {
    wButtons[i]->move(x, button_rc.top);
    x += d;
  }

  // Show MessageBox and save result
  res = wf.ShowModal();

  delete wText;
  for (int i = 0; i < ButtonCount; ++i)
    delete wButtons[i];
  wf.reset();

  return(res);
}
