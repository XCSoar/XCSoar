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

#include "Screen/ProgressWindow.hpp"
#include "InfoBoxLayout.h"
#include "Language.hpp"
#include "Version.hpp"
#include "resource.h"

#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>

#ifndef WINDOWSPC
#include "aygshell.h"
#endif

ProgressWindow::ProgressWindow(ContainerWindow &parent)
{
  set(parent,
      InfoBoxLayout::landscape
      ? (LPCTSTR)IDD_PROGRESS_LANDSCAPE
      : (LPCTSTR)IDD_PROGRESS);

  TCHAR Temp[1024];
  _stprintf(Temp, _T("%s %s"), gettext(_T("Version")), XCSoar_Version);
  set_item_text(IDC_VERSION, Temp);

#ifdef WINDOWSPC
  RECT rc = parent.get_client_rect();
  RECT rcp = get_client_rect();

  move(rc.left, rc.top, rcp.right - rcp.left, rcp.bottom - rcp.top);
#else
#ifndef GNAV
  SHFullScreen(hWnd, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
#endif

  insert_after(HWND_TOP, true);

  set_range(0, 100);
  set_step(5);

#ifndef ENABLE_SDL
  ::SetForegroundWindow(hWnd);
#endif /* !ENABLE_SDL */
  update();
}

void
ProgressWindow::set_message(const TCHAR *text)
{
  set_item_text(IDC_MESSAGE, text);
  update();
}

void
ProgressWindow::set_range(unsigned min_value, unsigned max_value)
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  ::SendMessage(get_item(IDC_PROGRESS1),
                PBM_SETRANGE, (WPARAM)0,
                (LPARAM)MAKELPARAM(min_value, max_value));
#endif /* !ENABLE_SDL */
}

void
ProgressWindow::set_step(unsigned size)
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  ::SendMessage(get_item(IDC_PROGRESS1),
                PBM_SETSTEP, (WPARAM)size, (LPARAM)0);
#endif /* !ENABLE_SDL */
}

void
ProgressWindow::set_pos(unsigned value)
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  ::SendMessage(get_item(IDC_PROGRESS1), PBM_SETPOS,
                value, 0);
#endif /* !ENABLE_SDL */
  update();
}

void
ProgressWindow::step()
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  ::SendMessage(get_item(IDC_PROGRESS1), PBM_STEPIT,
                (WPARAM)0, (LPARAM)0);
#endif /* !ENABLE_SDL */
  update();
}

bool
ProgressWindow::on_initdialog()
{
  Dialog::on_initdialog();

#ifdef WINDOWSPC
  move(0, 0);
#endif

  return true;
}

bool
ProgressWindow::on_erase(Canvas &canvas)
{
  canvas.white_pen();
  canvas.white_brush();

  RECT rc = get_client_rect();
  canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
  return true;
}

bool
ProgressWindow::on_command(HWND hWnd, unsigned id, unsigned code)
{
  if (id == IDOK) {
    end(id);
    return true;
  }

  return false;
}
