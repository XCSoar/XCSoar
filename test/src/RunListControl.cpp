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

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Form/List.hpp"
#include "Form/Form.hpp"
#include "Screen/Canvas.hpp"

static void
PaintItemCallback(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  TCHAR text[32];
  _stprintf(text, _T("%u"), idx);
  canvas.DrawText(rc.left + 2, rc.top + 2, text);
}

static void
Main()
{
  WndForm form(*dialog_look);
  form.Create(main_window, _T("RunListControl"));
  ContainerWindow &client_area = form.GetClientAreaWindow();

  PixelRect list_rc = client_area.GetClientRect();
  list_rc.Grow(-2);

  WindowStyle style;
  style.TabStop();
  ListControl list(client_area, *dialog_look, list_rc,
                   style, normal_font.GetHeight() + 4);

  FunctionListItemRenderer renderer(PaintItemCallback);
  list.SetItemRenderer(&renderer);
  list.SetLength(64);
  list.SetFocus();

  form.ShowModal();
}
