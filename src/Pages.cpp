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

#include "Pages.hpp"

#include "Interface.hpp"
#include "MainWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

namespace Pages
{
  int Current = 0;
}

void
Pages::Next()
{
  Current++;
  if (Current > 2)
    Current = 0;

  Update();
}

void
Pages::Prev()
{
  Current--;
  if (Current < 0)
    Current = 2;

  Update();
}

void
Pages::Open(int page)
{
  Current = page;
  Update();
}

void
Pages::Update()
{
  switch (Current) {
  case 1:
    OpenLayout(lMapAuxInfoBoxes);
    break;

  case 2:
    OpenLayout(lMap);
    break;

  case 0:
  default:
    OpenLayout(lMapInfoBoxes);
    break;
  }
}

void
Pages::OpenLayout(Layout layout)
{
  switch (layout) {
  case lMapInfoBoxes:
    XCSoarInterface::main_window.SetFullScreen(false);
    XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = false;
    break;

  case lMapAuxInfoBoxes:
    XCSoarInterface::main_window.SetFullScreen(false);
    XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = true;
    break;

  case lMap:
    XCSoarInterface::main_window.SetFullScreen(true);
    XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = false;
    break;
  }

  InfoBoxManager::SetDirty();
  XCSoarInterface::SendSettingsMap(true);
}
