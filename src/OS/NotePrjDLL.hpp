/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_NOTEPRJDLL_HPP
#define XCSOAR_NOTEPRJDLL_HPP

#include "DynamicLibrary.hpp"

class NotePrjDLL : public DynamicLibrary {
protected:
  typedef HANDLE WINAPI (*FindFirstFlashCard_t)(LPWIN32_FIND_DATA lpFindFlashData);
  typedef BOOL WINAPI (*FindNextFlashCard_t)(HANDLE hFlashCard,
                                      LPWIN32_FIND_DATA lpFindFlashData);

  FindFirstFlashCard_t FindFirstFlashCard_p;
  FindNextFlashCard_t FindNextFlashCard_p;

public:
  NotePrjDLL()
    :DynamicLibrary(_T("note_prj")),
     FindFirstFlashCard_p((FindFirstFlashCard_t)Lookup(_T("FindFirstFlashCard"))),
     FindNextFlashCard_p((FindNextFlashCard_t)Lookup(_T("FindNextFlashCard"))) {}

  HANDLE FindFirstFlashCard(LPWIN32_FIND_DATA lpFindFlashData) const {
    return FindFirstFlashCard_p != NULL
      ? FindFirstFlashCard_p(lpFindFlashData)
      : NULL;
  }

  BOOL FindNextFlashCard(HANDLE hFlashCard,
                          LPWIN32_FIND_DATA lpFindFlashData) const {
    return FindNextFlashCard_p != NULL
      ? FindNextFlashCard_p(hFlashCard, lpFindFlashData)
      : NULL;
  }
};

#endif
