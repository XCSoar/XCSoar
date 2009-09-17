/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Screen/Dialog.hpp"
#include "Interface.hpp" /* for XCSoarInterface::hInst */

#include <assert.h>

void
Dialog::set(ContainerWindow &_parent, LPCTSTR template_name)
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  hWnd = ::CreateDialogParam(XCSoarInterface::hInst, template_name, _parent,
                             DlgProc, (LPARAM)this);
#endif /* !ENABLE_SDL */
}

bool
Dialog::on_initdialog()
{
  return true;
}

#ifndef ENABLE_SDL

LRESULT
Dialog::on_unhandled_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
{
  return 1;
}

LRESULT
Dialog::on_message(HWND _hWnd, UINT message,
                   WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_INITDIALOG:
    if (on_initdialog())
      return 0;
    break;
  };

  return ContainerWindow::on_message(_hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK
Dialog::DlgProc(HWND hwndDlg, UINT message,
                WPARAM wParam, LPARAM lParam)
{
  Dialog *dialog;

  if (message == WM_INITDIALOG) {
    dialog = (Dialog *)lParam;
    dialog->created(hwndDlg);
    dialog->set_userdata((Window *)dialog);
  } else {
    dialog = (Dialog *)get_unchecked(hwndDlg);

    if (dialog == NULL && message == WM_SETFONT)
      return false;
  }

  assert(dialog != NULL);

  return dialog->on_message(hwndDlg, message, wParam, lParam) == 0;
}

#endif /* !ENABLE_SDL */
