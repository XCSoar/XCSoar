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

#include "Screen/Layout.hpp"
#include "Form/Form.hpp"
#include "Form/Container.hpp"
#include "Form/KeyboardButton.hpp"

static TCHAR keyboard_letters[] = _T("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");

KeyboardControl::KeyboardControl(WndForm &form, ContainerControl *owner,
                                 int x, int y, unsigned width, unsigned height,
                                 const Font *font,
                                 const WindowStyle _style) :
  PanelControl(owner, x, y, width, height, _style),
  parent_form(form)
{
  for (unsigned i = 0;
       i < sizeof(keyboard_letters) / sizeof(keyboard_letters[0]) - 1; i++) {
    TCHAR caption[2];
    _tcsncpy(caption, keyboard_letters + i, 1);
    caption[1] = 0;

    TCHAR name[5] = _T("cmd");
    _tcsncat(name, keyboard_letters + i, 1);
    name[4] = 0;

    add_button(form, name, caption, font);
  }

  add_button(form, _T("cmdSpace"), _T(" Space "), font);
  add_button(form, _T("cmdPeriod"), _T("."), font);
  add_button(form, _T("cmdComma"), _T(","), font);
  add_button(form, _T("cmdMinus"), _T("-"), font);

  move_buttons();
}

KeyboardButton*
KeyboardControl::get_button(const TCHAR* name)
{
  return (KeyboardButton*)parent_form.FindByName(name);
}

void
KeyboardControl::move_button(const TCHAR* name, int left, int top)
{
  KeyboardButton* kb = get_button(name);
  if (kb)
    kb->move(left, top);
}

void
KeyboardControl::resize_button(const TCHAR* name,
                               unsigned int width, unsigned int height)
{
  KeyboardButton* kb = get_button(name);
  if (kb)
    kb->resize(width, height);
}

void
KeyboardControl::move_buttons()
{
  if (Layout::landscape) {
    move_button(_T("cmd1"), Layout::FastScale(0), Layout::FastScale(0));
    move_button(_T("cmd2"), Layout::FastScale(32), Layout::FastScale(0));
    move_button(_T("cmd3"), Layout::FastScale(64), Layout::FastScale(0));
    move_button(_T("cmd4"), Layout::FastScale(96), Layout::FastScale(0));
    move_button(_T("cmd5"), Layout::FastScale(128), Layout::FastScale(0));
    move_button(_T("cmd6"), Layout::FastScale(160), Layout::FastScale(0));
    move_button(_T("cmd7"), Layout::FastScale(192), Layout::FastScale(0));
    move_button(_T("cmd8"), Layout::FastScale(224), Layout::FastScale(0));
    move_button(_T("cmd9"), Layout::FastScale(256), Layout::FastScale(0));
    move_button(_T("cmd0"), Layout::FastScale(288), Layout::FastScale(0));

    move_button(_T("cmdQ"), Layout::FastScale(0), Layout::FastScale(40));
    move_button(_T("cmdW"), Layout::FastScale(32), Layout::FastScale(40));
    move_button(_T("cmdE"), Layout::FastScale(64), Layout::FastScale(40));
    move_button(_T("cmdR"), Layout::FastScale(96), Layout::FastScale(40));
    move_button(_T("cmdT"), Layout::FastScale(128), Layout::FastScale(40));
    move_button(_T("cmdY"), Layout::FastScale(160), Layout::FastScale(40));
    move_button(_T("cmdU"), Layout::FastScale(192), Layout::FastScale(40));
    move_button(_T("cmdI"), Layout::FastScale(224), Layout::FastScale(40));
    move_button(_T("cmdO"), Layout::FastScale(256), Layout::FastScale(40));
    move_button(_T("cmdP"), Layout::FastScale(288), Layout::FastScale(40));

    move_button(_T("cmdA"), Layout::FastScale(10), Layout::FastScale(80));
    move_button(_T("cmdS"), Layout::FastScale(42), Layout::FastScale(80));
    move_button(_T("cmdD"), Layout::FastScale(74), Layout::FastScale(80));
    move_button(_T("cmdF"), Layout::FastScale(106), Layout::FastScale(80));
    move_button(_T("cmdG"), Layout::FastScale(138), Layout::FastScale(80));
    move_button(_T("cmdH"), Layout::FastScale(170), Layout::FastScale(80));
    move_button(_T("cmdJ"), Layout::FastScale(202), Layout::FastScale(80));
    move_button(_T("cmdK"), Layout::FastScale(234), Layout::FastScale(80));
    move_button(_T("cmdL"), Layout::FastScale(266), Layout::FastScale(80));

    move_button(_T("cmdZ"), Layout::FastScale(20), Layout::FastScale(120));
    move_button(_T("cmdX"), Layout::FastScale(52), Layout::FastScale(120));
    move_button(_T("cmdC"), Layout::FastScale(84), Layout::FastScale(120));
    move_button(_T("cmdV"), Layout::FastScale(116), Layout::FastScale(120));
    move_button(_T("cmdB"), Layout::FastScale(148), Layout::FastScale(120));
    move_button(_T("cmdN"), Layout::FastScale(180), Layout::FastScale(120));
    move_button(_T("cmdM"), Layout::FastScale(212), Layout::FastScale(120));

    move_button(_T("cmdComma"), Layout::FastScale(244), Layout::FastScale(120));
    move_button(_T("cmdPeriod"), Layout::FastScale(276), Layout::FastScale(120));
    move_button(_T("cmdMinus"), Layout::FastScale(285), Layout::FastScale(160));

    move_button(_T("cmdSpace"), Layout::FastScale(81), Layout::FastScale(160));
    resize_button(_T("cmdSpace"), Layout::FastScale(97), Layout::FastScale(40));
  } else {
    move_button(_T("cmd1"), Layout::FastScale(0), Layout::FastScale(0));
    move_button(_T("cmd2"), Layout::FastScale(24), Layout::FastScale(0));
    move_button(_T("cmd3"), Layout::FastScale(48), Layout::FastScale(0));
    move_button(_T("cmd4"), Layout::FastScale(72), Layout::FastScale(0));
    move_button(_T("cmd5"), Layout::FastScale(96), Layout::FastScale(0));
    move_button(_T("cmd6"), Layout::FastScale(120), Layout::FastScale(0));
    move_button(_T("cmd7"), Layout::FastScale(144), Layout::FastScale(0));
    move_button(_T("cmd8"), Layout::FastScale(168), Layout::FastScale(0));
    move_button(_T("cmd9"), Layout::FastScale(192), Layout::FastScale(0));
    move_button(_T("cmd0"), Layout::FastScale(216), Layout::FastScale(0));

    move_button(_T("cmdQ"), Layout::FastScale(0), Layout::FastScale(50));
    move_button(_T("cmdW"), Layout::FastScale(24), Layout::FastScale(50));
    move_button(_T("cmdE"), Layout::FastScale(48), Layout::FastScale(50));
    move_button(_T("cmdR"), Layout::FastScale(72), Layout::FastScale(50));
    move_button(_T("cmdT"), Layout::FastScale(96), Layout::FastScale(50));
    move_button(_T("cmdY"), Layout::FastScale(120), Layout::FastScale(50));
    move_button(_T("cmdU"), Layout::FastScale(144), Layout::FastScale(50));
    move_button(_T("cmdI"), Layout::FastScale(168), Layout::FastScale(50));
    move_button(_T("cmdO"), Layout::FastScale(192), Layout::FastScale(50));
    move_button(_T("cmdP"), Layout::FastScale(216), Layout::FastScale(50));

    move_button(_T("cmdA"), Layout::FastScale(8), Layout::FastScale(100));
    move_button(_T("cmdS"), Layout::FastScale(32), Layout::FastScale(100));
    move_button(_T("cmdD"), Layout::FastScale(56), Layout::FastScale(100));
    move_button(_T("cmdF"), Layout::FastScale(80), Layout::FastScale(100));
    move_button(_T("cmdG"), Layout::FastScale(104), Layout::FastScale(100));
    move_button(_T("cmdH"), Layout::FastScale(128), Layout::FastScale(100));
    move_button(_T("cmdJ"), Layout::FastScale(152), Layout::FastScale(100));
    move_button(_T("cmdK"), Layout::FastScale(176), Layout::FastScale(100));
    move_button(_T("cmdL"), Layout::FastScale(200), Layout::FastScale(100));

    move_button(_T("cmdZ"), Layout::FastScale(16), Layout::FastScale(150));
    move_button(_T("cmdX"), Layout::FastScale(40), Layout::FastScale(150));
    move_button(_T("cmdC"), Layout::FastScale(64), Layout::FastScale(150));
    move_button(_T("cmdV"), Layout::FastScale(88), Layout::FastScale(150));
    move_button(_T("cmdB"), Layout::FastScale(112), Layout::FastScale(150));
    move_button(_T("cmdN"), Layout::FastScale(136), Layout::FastScale(150));
    move_button(_T("cmdM"), Layout::FastScale(160), Layout::FastScale(150));

    move_button(_T("cmdComma"), Layout::FastScale(184), Layout::FastScale(150));
    move_button(_T("cmdPeriod"), Layout::FastScale(208), Layout::FastScale(150));
    move_button(_T("cmdMinus"), Layout::FastScale(208), Layout::FastScale(197));

    move_button(_T("cmdSpace"), Layout::FastScale(72), Layout::FastScale(197));
    resize_button(_T("cmdSpace"), Layout::FastScale(96), Layout::FastScale(40));
  }
}

void
KeyboardControl::on_keyboard_button(const TCHAR* caption)
{
  if (mOnCharacter == NULL)
    return;

  if (_tcscmp(caption, _T(" Space ")) == 0 || _tcscmp(caption, _T(" ")) == 0)
    mOnCharacter(_T(' '));
  if (_tcscmp(caption, _T(".")) == 0)
    mOnCharacter(_T('.'));
  if (_tcscmp(caption, _T(",")) == 0)
    mOnCharacter(_T(','));
  if (_tcscmp(caption, _T("-")) == 0)
    mOnCharacter(_T('-'));

  if (_tcslen(caption) == 1 && _tcschr(keyboard_letters, caption[0]) != NULL)
    mOnCharacter(caption[0]);
}

void
KeyboardControl::add_button(WndForm &form, const TCHAR* name,
                            const TCHAR* caption, const Font *font)
{
  WindowStyle style;
  style.tab_stop();

  KeyboardButton *button = NULL;
  if (Layout::landscape)
    button = new KeyboardButton(this, caption, 0, 0, Layout::FastScale(32),
                                Layout::FastScale(40), style);
  else
    button = new KeyboardButton(this, caption, 0, 0, Layout::FastScale(24),
                                Layout::FastScale(40), style);
  button->SetFont(font);
  form.AddNamed(name, button);
  form.AddDestruct(button);
}
