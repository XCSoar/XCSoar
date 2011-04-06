/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FLASH_CARD_ENUMERATOR_HPP
#define XCSOAR_FLASH_CARD_ENUMERATOR_HPP

#include "Util/StringUtil.hpp"
#include "Compatibility/path.h"

#ifdef HAVE_NOTE_PRJ_DLL
#include "NotePrjDLL.hpp"
#endif

#include <windows.h>

class FlashCardEnumerator {
#ifdef HAVE_NOTE_PRJ_DLL
  const NotePrjDLL note_prj;
#endif

  HANDLE handle;
  WIN32_FIND_DATA data;
  bool first;

public:
#ifdef HAVE_NOTE_PRJ_DLL
  FlashCardEnumerator()
    :handle(note_prj.defined()
            ? note_prj.FindFirstFlashCard(&data)
            : ::FindFirstFile(_T(DIR_SEPARATOR_S "*"), &data)),
     first(true) {}
#else
  FlashCardEnumerator()
    :handle(::FindFirstFile(_T(DIR_SEPARATOR_S "*"), &data)), first(true) {}
#endif

  ~FlashCardEnumerator() {
    if (handle != INVALID_HANDLE_VALUE)
      ::FindClose(handle);
  }

  const TCHAR *next() {
    if (handle == INVALID_HANDLE_VALUE)
      return NULL;

#ifdef HAVE_NOTE_PRJ_DLL
    if (note_prj.defined()) {
      if (first)
        first = false;
      else if (!note_prj.FindNextFlashCard(handle, &data) ||
               /* On my Dell Axim x51v, the last entry returned by
                  FindNextFlashCard() is empty; workaround: */
               string_is_empty(data.cFileName))
        return NULL;
      return data.cFileName;
    }
#endif

    enum {
      /* Directories with the "TEMPORARY" flag set seem to be
         mountable flash drives - use this trick on platforms where
         note_prj.dll is unavailable */
      FLASH = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_TEMPORARY,
    };

    do {
      if (first)
        first = false;
      else if (!::FindNextFile(handle, &data))
        return NULL;
    } while ((data.dwFileAttributes & FLASH) != FLASH);

    return data.cFileName;
  }
};

#endif
