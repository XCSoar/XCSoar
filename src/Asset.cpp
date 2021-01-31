/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Asset.hpp"

#ifdef USE_CONSOLE
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#include <string.h>

// Registration Data
TCHAR asset_number[100] = _T(""); //4G17DW31L0HY");

void
ReadAssetNumber()
{
  _tcscpy(asset_number, _T("AAA"));
}

#if defined(USE_CONSOLE) && !defined(KOBO)

bool
HasPointer()
{
  return UI::event_queue->HasPointer();
}

#endif

#ifdef USE_LIBINPUT

bool
HasTouchScreen()
{
  return UI::event_queue->HasTouchScreen();
}

bool
HasKeyboard()
{
  return UI::event_queue->HasKeyboard();
}

#endif
