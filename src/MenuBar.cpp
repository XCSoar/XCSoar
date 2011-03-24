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

#ifdef GREEN_MENU
static const Color menu_button_bk_normal(0xa0, 0xe0, 0xa0);
static const Color menu_button_bk_down(0x80, 0xc0, 0x80);
static const Color menu_button_bk_disabled(0xb0, 0xc0, 0xb0);
#endif

static void
GetButtonPosition(unsigned i, PixelRect rc, int *x, int *y, int *sizex, int *sizey)
{
  unsigned hwidth = rc.right - rc.left;
  unsigned hheight = rc.bottom - rc.top;

  const unsigned margin = Layout::FastScale(2);

  if (hheight > hwidth) {
    // portrait

    hheight /= 6;

    *sizex = hwidth - margin * 2;
    *sizey = hheight - margin * 2;

    if (i == 0) {
      *sizex = IBLSCALE(52);
      *sizey = IBLSCALE(37);
      *x = rc.left - (*sizex); // JMW make it offscreen for now
      *y = rc.bottom - (*sizey);
    } else if (i < 5) {
      hwidth /= 4;

      *sizex = hwidth - margin * 2;
      *sizey = hheight - margin * 2;
      *x = rc.left + margin + hwidth * (i - 1);
      *y = rc.bottom - hheight + margin;
    } else {
      hwidth /= 3;

      *sizex = hwidth - margin * 2;
      *sizey = hheight - margin * 2;

      *x = rc.right - hwidth + margin;
      int k = rc.bottom - rc.top - IBLSCALE(46);

      if (is_altair()) {
        k = rc.bottom - rc.top;
        // JMW need upside down button order for rotated Altair
        *y = rc.bottom - (i - 5) * k / 5 - (*sizey) - IBLSCALE(20);
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

#ifdef GREEN_MENU

static Color
menu_button_bk_color(bool enabled, bool down)
{
  if (!enabled)
    return menu_button_bk_disabled;

  return down
    ? menu_button_bk_down
    : menu_button_bk_normal;
}

void
MenuBar::Button::on_paint(Canvas &canvas)
{
  canvas.fill_rectangle(0, 0, canvas.get_width(), canvas.get_height(),
                        menu_button_bk_color(is_enabled(), is_down()));

  canvas.set_text_color(is_enabled() ? Color::BLACK : Color::GRAY);
  canvas.background_transparent();

#ifndef ENABLE_SDL
  HFONT font = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
  if (font != NULL)
    ::SelectObject(canvas, font);
#endif

  PixelRect rc = get_client_rect();
  canvas.formatted_text(&rc, get_text().c_str(),
#ifdef ENABLE_SDL
                        DT_VCENTER |
#endif
                        DT_NOPREFIX | DT_CENTER |
                        DT_NOCLIP | DT_WORDBREAK);
}

#endif /* GREEN_MENU */

bool
MenuBar::Button::on_clicked()
{
  InputEvents::processButton(index);
  return true;
}

#ifndef ENABLE_SDL
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
