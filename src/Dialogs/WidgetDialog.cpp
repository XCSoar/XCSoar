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
#include "Screen/SingleWindow.hpp"

gcc_const
static WindowStyle
GetDialogStyle()
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();
  return style;
}

WidgetDialog::WidgetDialog(const TCHAR *caption, const PixelRect &rc,
                           Widget *_widget)
  :WndForm(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
           rc, caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), UIGlobals::GetDialogLook()),
   widget(GetClientAreaWindow(), _widget),
   auto_size(false),
   changed(false)
{
  widget.Move(buttons.UpdateLayout());
}

WidgetDialog::WidgetDialog(const TCHAR *caption, Widget *_widget)
  :WndForm(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
           UIGlobals::GetMainWindow().get_client_rect(),
           caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), UIGlobals::GetDialogLook()),
   widget(GetClientAreaWindow(), _widget),
   auto_size(true),
   changed(false)
{
  widget.Move(buttons.UpdateLayout());
}

int
WidgetDialog::ShowModal()
{
  PixelRect remaining = buttons.UpdateLayout();

  if (auto_size) {
    widget.Prepare();

    const PixelSize this_size = get_size();
    const PixelSize cur_size = {
      PixelScalar(remaining.right - remaining.left),
      PixelScalar(remaining.bottom - remaining.top),
    };
    const PixelSize max_size = widget.Get()->GetMaximumSize();
    PixelSize new_size = cur_size;

    if (max_size.cx > 0 && max_size.cx < new_size.cx)
      new_size.cx = max_size.cx;

    if (max_size.cy > 0 && max_size.cy < new_size.cy)
      new_size.cy = max_size.cy;

    resize(new_size.cx + (this_size.cx - cur_size.cx),
           new_size.cy + (this_size.cy - cur_size.cy));
  }

  widget.Show();
  return WndForm::ShowModal();
}

void
WidgetDialog::OnAction(int id)
{
  if (id == mrOK) {
    bool require_restart;
    if (!widget.Get()->Save(changed, require_restart))
      return;
  }

  WndForm::OnAction(id);
}

void
WidgetDialog::OnDestroy()
{
  widget.Unprepare();
}

void
WidgetDialog::OnResize(UPixelScalar width, UPixelScalar height)
{
  WndForm::OnResize(width, height);
  widget.Move(buttons.UpdateLayout());
}

bool
DefaultWidgetDialog(const TCHAR *caption, const PixelRect &rc, Widget &widget)
{
  WidgetDialog dialog(caption, rc, &widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}

bool
DefaultWidgetDialog(const TCHAR *caption, Widget &widget)
{
  WidgetDialog dialog(caption, &widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}
