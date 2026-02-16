// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Form/Form.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/control/List.hpp"

static void
PaintItemCallback(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  char text[32];
  _stprintf(text, _T("%u"), idx);
  canvas.DrawText(rc.WithPadding(2).GetTopLeft(), text);
}

static void
Main(TestMainWindow &main_window)
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
