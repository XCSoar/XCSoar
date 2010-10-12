/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Form/Keyboard.hpp"
#include "StringUtil.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Fonts.hpp"

#include <assert.h>
#include <string.h>

static const TCHAR keyboard_letters[] =
  _T("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");

KeyboardControl::KeyboardControl(ContainerWindow &parent,
                                 int x, int y, unsigned width, unsigned height,
                                 Color _background_color,
                                 OnCharacterCallback_t function,
                                 const WindowStyle _style) :
  num_buttons(0),
  background_color(_background_color),
  button_width(50), button_height(50),
  mOnCharacter(function)
{
  set(parent, x, y, width, height, _style);
  set_buttons_size();

  TCHAR caption[] = _T(" ");

  for (const TCHAR *i = keyboard_letters; !string_is_empty(i); ++i) {
    caption[0] = *i;

    add_button(caption);
  }

  add_button(_T(" Space "));
  add_button(_T("."));
  add_button(_T(","));
  add_button(_T("-"));

  move_buttons();
}

void
KeyboardControl::SetAllowedCharacters(const TCHAR *allowed)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].set_visible(allowed == NULL ||
                           _tcschr(allowed, button_values[i]) != NULL);
}

ButtonWindow *
KeyboardControl::get_button(TCHAR ch)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (button_values[i] == ch)
      return &buttons[i];

  return NULL;

}

void
KeyboardControl::move_button(TCHAR ch, int left, int top)
{
  ButtonWindow *kb = get_button(ch);
  if (kb)
    kb->move(left, top);
}

void
KeyboardControl::resize_button(TCHAR ch,
                               unsigned int width, unsigned int height)
{
  ButtonWindow *kb = get_button(ch);
  if (kb)
    kb->resize(width, height);
}

void
KeyboardControl::resize_buttons()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].resize(button_width, button_height);
}

void
KeyboardControl::set_buttons_size()
{
  button_width = get_width() / 10;
  button_height = get_height() / 5;
}

void
KeyboardControl::move_buttons_to_row(const TCHAR* buttons, int row, int offset)
{
  if (string_is_empty(buttons))
    return;

  ButtonWindow *kb;
  for (unsigned i = 0; i < _tcslen(buttons); i++) {
    kb = get_button(buttons[i]);
    if (!kb)
      continue;

    kb->move(i * button_width + offset, row * button_height);
  }
}

void
KeyboardControl::move_buttons()
{
  move_buttons_to_row(_T("1234567890"), 0);
  move_buttons_to_row(_T("QWERTYUIOP"), 1);
  move_buttons_to_row(_T("ASDFGHJKL"), 2, button_width / 3);
  move_buttons_to_row(_T("ZXCVBNM,."), 3, button_width * 2 / 3);

  if (is_landscape()) {
    move_button(_T('-'), button_width * 9, button_height * 4);

    move_button(_T(' '), button_width * 5 / 2, button_height * 4);
    resize_button(_T(' '), button_width * 3, button_height);
  } else {
    move_button(_T('-'), button_width * 8, button_height * 4);

    move_button(_T(' '), button_width * 2, button_height * 4);
    resize_button(_T(' '), button_width * 11 / 2, button_height);
  }
}

void
KeyboardControl::on_paint(Canvas &canvas)
{
  canvas.clear(background_color);
}

bool
KeyboardControl::on_command(unsigned id, unsigned code)
{
  if (id >= 0x20 && mOnCharacter != NULL) {
    mOnCharacter((TCHAR)id);
    return true;
  } else
    return ContainerWindow::on_command(id, code);
}

bool
KeyboardControl::on_resize(unsigned width, unsigned height)
{
  set_buttons_size();
  resize_buttons();
  move_buttons();
  return true;
}

bool
KeyboardControl::is_landscape() {
  return get_width() >= get_height();
}

void
KeyboardControl::add_button(const TCHAR* caption)
{
  assert(num_buttons < MAX_BUTTONS);

  button_values[num_buttons] = caption[0];
  ButtonWindow *button = &buttons[num_buttons++];
  button->set(*this, caption, (unsigned)caption[0],
              0, 0, button_width, button_height);
  button->set_font(Fonts::MapBold);
}
