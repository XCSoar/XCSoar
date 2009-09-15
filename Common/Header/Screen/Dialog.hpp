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

#ifndef XCSOAR_SCREEN_DIALOG_HXX
#define XCSOAR_SCREEN_DIALOG_HXX

#include "Screen/ContainerWindow.hpp"

#if !defined(WINDOWSPC) && !defined(GNAV) && (UNDER_CE >= 300 || _WIN32_WCE >= 0x0300)
#define HAVE_ACTIVATE_INFO
#endif

#ifdef HAVE_ACTIVATE_INFO
#include <aygshell.h>
#endif

/**
 * A top-level full-screen window.
 */
class Dialog : public ContainerWindow {
public:
  void set(ContainerWindow &_parent, LPCTSTR template_name);

  void end(int result) {
    ::EndDialog(hWnd, result);
  }

protected:
  virtual bool on_initdialog();

  virtual LRESULT on_unhandled_message(HWND hWnd, UINT message,
                                       WPARAM wParam, LPARAM lParam);

  virtual LRESULT on_message(HWND _hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);

  static INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam);
};

#endif
