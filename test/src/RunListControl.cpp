/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Form/List.hpp"
#include "Form/Form.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Init.hpp"
#include "Look/DialogLook.hpp"
#include "Fonts.hpp"

static void
PaintItemCallback(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  TCHAR text[32];
  _stprintf(text, _T("%u"), idx);
  canvas.text(rc.left + 2, rc.top + 2, text);
}

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  PixelRect screen_rc{0, 0, 640, 480};

  ScreenGlobalInit screen_init;
  Layout::Initialize(screen_rc.right - screen_rc.left,
                     screen_rc.bottom - screen_rc.top);

  InitialiseFonts();
  DialogLook *dialog_look = new DialogLook();
  dialog_look->Initialise(bold_font, normal_font, small_font,
                          bold_font, bold_font);

  SingleWindow main_window;
  main_window.set(_T("STATIC"), _T("RunListControl"), screen_rc);
  main_window.Show();

  WndForm form(main_window, *dialog_look, main_window.GetClientRect(),
               _T("RunListControl"));
  ContainerWindow &client_area = form.GetClientAreaWindow();

  PixelRect list_rc = client_area.GetClientRect();
  GrowRect(list_rc, -2, -2);

  WindowStyle style;
  style.TabStop();
  ListControl list(client_area, *dialog_look, list_rc,
                   style, normal_font.GetHeight() + 4);
  list.SetPaintItemCallback(PaintItemCallback);
  list.SetLength(64);
  list.SetFocus();

  form.ShowModal();

  delete dialog_look;
  DeinitialiseFonts();

  return 0;
}
