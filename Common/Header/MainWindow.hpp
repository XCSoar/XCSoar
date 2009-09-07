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

#ifndef XCSOAR_MAIN_WINDOW_HXX
#define XCSOAR_MAIN_WINDOW_HXX

#include "Screen/TopWindow.hpp"
#include "MapWindow.h"

class GaugeVario;
class GaugeFLARM;

/**
 * The XCSoar main window.
 */
class MainWindow : public TopWindow {
public:
  MapWindow map;
  GaugeVario *vario;
  GaugeFLARM *flarm;

public:
  MainWindow():vario(NULL), flarm(NULL) {}
  virtual ~MainWindow();

  static bool find(LPCTSTR text) {
    return TopWindow::find(_T("XCSoarMain"), text);
  }

  static bool register_class(HINSTANCE hInstance);

  void set(LPCTSTR text,
           int left, int top, unsigned width, unsigned height);

  void reset() {
    map.reset();
    TopWindow::reset();
  }

protected:
  virtual LRESULT on_message(HWND _hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);

  LRESULT on_colour(HDC hdc, int wdata);
  int _timer_id;
  virtual bool on_command(HWND hWnd, unsigned id, unsigned code);
  bool on_activate();
  bool on_timer(unsigned id);
  bool on_create();
  bool on_destroy();
  bool on_close();
public:
  void install_timer();
};

#endif
