/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "MenuBar.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Layout.hpp"
#include "InputEvents.hpp"

#include <assert.h>

static void
GetButtonPosition(unsigned i, PixelRect rc, int *x, int *y, int *sizex, int *sizey)
{
  unsigned hwidth = rc.right - rc.left;
  unsigned hheight = rc.bottom - rc.top;

  const unsigned margin = Layout::FastScale(2);

  if (hheight > hwidth) {
    // portrait

    hheight /= 6;

    *sizey = hheight - margin * 2;

    if (i == 0) {
      *sizex = Layout::Scale(52);
      *sizey = Layout::Scale(37);
      *x = rc.left - (*sizex); // JMW make it offscreen for now
      *y = rc.bottom - (*sizey);
    } else if (i < 5) {
      hwidth /= 4;

      *sizex = hwidth - margin * 2;
      *x = rc.left + margin + hwidth * (i - 1);
      *y = rc.bottom - hheight + margin;
    } else {
      hwidth /= 3;

      *sizex = hwidth - margin * 2;
      *x = rc.right - hwidth + margin;
      int k = rc.bottom - rc.top - Layout::Scale(46);

      if (is_altair()) {
        k = rc.bottom - rc.top;
        // JMW need upside down button order for rotated Altair
        *y = rc.bottom - (i - 5) * k / 5 - (*sizey) - Layout::Scale(20);
      } else {
        *y = rc.top + (i - 5) * hheight + margin;
      }
    }
  } else {
    // landscape

    hwidth /= 5;
    hheight /= 5;

    *sizex = hwidth - margin * 2;
    *sizey = hheight - margin * 2;

    if (i == 0) {
      *x = rc.left - (*sizex); // JMW make it offscreen for now
      *y = (rc.top);
    } else if (i < 5) {
      *x = rc.left + margin;
      *y = rc.top + margin + hheight * (i - 1);
    } else {
      *x = rc.left + hwidth * (i - 5);
      *y = rc.bottom - hheight + margin;
    }
  }
}

bool
MenuBar::Button::on_clicked()
{
  InputEvents::processButton(index);
  return true;
}

#ifdef USE_GDI
LRESULT
MenuBar::Button::on_message(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CAPTURECHANGED:
    if (lParam == 0)
      /* the button has the keyboard focus, and the user has stopped
         dragging the mouse: return the keyboard focus to the parent
         window, because menu buttons shouldn't have keyboard focus */
      ::SetFocus(::GetParent(hWnd));
    break;
  }

  return ButtonWindow::on_message(hWnd, message, wParam, lParam);
}
#endif

MenuBar::MenuBar(ContainerWindow &parent)
{
  const PixelRect rc = parent.get_client_rect();
  int x, y, xsize, ysize;

  ButtonWindowStyle style;
  style.hide();
  style.border();
  style.multiline();

  for (unsigned i = 0; i < MAX_BUTTONS; ++i) {
    GetButtonPosition(i, rc, &x, &y, &xsize, &ysize);
    buttons[i].set(parent, _T(""), i, x, y, xsize, ysize,
                   style);
  }
}

void
MenuBar::SetFont(const Font &font)
{
  for (unsigned i = 0; i < MAX_BUTTONS; i++)
    buttons[i].set_font(font);
}

void
MenuBar::ShowButton(unsigned i, bool enabled, const TCHAR *text)
{
  assert(i < MAX_BUTTONS);

  ButtonWindow &button = buttons[i];

  button.set_text(text);
  button.set_enabled(enabled);
  button.show_on_top();
}

void
MenuBar::HideButton(unsigned i)
{
  assert(i < MAX_BUTTONS);

  buttons[i].hide();
}
