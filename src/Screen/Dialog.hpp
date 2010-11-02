/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_DIALOG_HXX
#define XCSOAR_SCREEN_DIALOG_HXX

#include "Screen/ContainerWindow.hpp"

/**
 * A top-level full-screen window.
 */
class Dialog : public ContainerWindow {
protected:
#ifndef ENABLE_SDL
  HWND get_item(int id) {
    return ::GetDlgItem(hWnd, id);
  }
#endif /* !ENABLE_SDL */

public:
  void set(ContainerWindow &_parent, const TCHAR *template_name);

  void end(int result) {
#ifdef ENABLE_SDL
    // XXX
#else
    ::EndDialog(hWnd, result);
#endif
  }

  void set_item_text(int id, const TCHAR *text) {
#ifdef ENABLE_SDL
    // XXX
#else
    ::SetWindowText(get_item(id), text);
#endif /* !ENABLE_SDL */
  }


protected:
  virtual bool on_initdialog();

#ifndef ENABLE_SDL
  virtual LRESULT on_unhandled_message(HWND hWnd, UINT message,
                                       WPARAM wParam, LPARAM lParam);

  virtual LRESULT on_message(HWND _hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);

  static INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam);
#endif /* !ENABLE_SDL */
};

#endif
