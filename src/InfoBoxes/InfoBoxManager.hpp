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

#ifndef XCSOAR_INFO_BOX_MANAGER_HPP
#define XCSOAR_INFO_BOX_MANAGER_HPP

#include "Screen/PaintWindow.hpp"
#include "InfoBoxes/Content/Base.hpp"

#include <windef.h>

extern unsigned numInfoWindows;

class InfoBoxWindow;

class InfoBoxFullWindow : public PaintWindow {
protected:
  virtual void on_paint(Canvas &canvas);
};

namespace InfoBoxManager
{
  enum mode {
    MODE_CIRCLING,
    MODE_CRUISE,
    MODE_FINAL_GLIDE,
    MODE_AUXILIARY,
  };

  void Event_Select(int i);
  void Event_Change(int i);

  const TCHAR *GetTypeDescription(unsigned i);

  void ProcessKey(InfoBoxContent::InfoBoxKeyCodes keycode);
  bool Click(InfoBoxWindow &ib);

  void ProcessTimer();
  void SetDirty();

  void Create(RECT rc);
  void Destroy();
  void Paint();
  void Show();
  void Hide();

  enum mode GetCurrentMode();

  unsigned GetType(unsigned box, enum mode mode);
  void SetType(unsigned box, char type, enum mode mode);

  unsigned GetTypes(unsigned box);
  void SetTypes(unsigned box, unsigned types);

  bool IsEmpty(enum mode mode);
  bool IsEmpty();

  bool HasFocus();

  /**
   * Opens a configuration dialog for the focused InfoBox.
   */
  void SetupFocused();
};

#endif
