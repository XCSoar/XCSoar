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
#ifndef INFOBOX_MANAGER_H
#define INFOBOX_MANAGER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern unsigned numInfoWindows;
extern const unsigned NUMSELECTSTRINGS;

#include "Interface.hpp"

class InfoBoxManager: public CommonInterface {
 private:
  static void FocusSetMaxTimeOut(void);
  static void ResetInfoBoxes(void);
  static int getType(unsigned i);
  static void setType(unsigned i, char j);
  static void FocusOnWindow(unsigned i, bool selected);
  static void DisplayInfoBox(void);
  static void InfoBoxDrawIfDirty();
  static void DestroyInfoBoxFormatters();
public:
  static void Event_Select(int i);
  static void Event_Change(int i);

  static TCHAR *GetTypeDescription(unsigned i);

  static void ProcessKey(int keycode);
  static bool Click(HWND wmControl);

  static void Focus();
  static bool Defocus(void);

  static void ProcessTimer(void);
  static void SetDirty(bool is_dirty);

  static RECT Create(RECT rc);
  static void Destroy(void);
  static void Paint(void);
  static void Show();
  static void Hide();

  static int getType(unsigned i, unsigned layer);
  static void setType(unsigned i, char j, unsigned layer);
  
  static int getTypeAll(unsigned i);
  static void setTypeAll(unsigned i, unsigned j);

  static bool IsFocus();
};

#endif
