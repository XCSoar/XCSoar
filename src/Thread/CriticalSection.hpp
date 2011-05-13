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

#ifndef XCSOAR_THREAD_CRITICAL_SECTION_HXX
#define XCSOAR_THREAD_CRITICAL_SECTION_HXX

#include "Util/NonCopyable.hpp"

#include <windows.h>

/**
 * Low-level wrapper for a CRITICAL_SECTION.
 */
class CriticalSection : private NonCopyable {
  CRITICAL_SECTION critical_section;

public:
  CriticalSection() {
    ::InitializeCriticalSection(&critical_section);
  }

  ~CriticalSection() {
    ::DeleteCriticalSection(&critical_section);
  }

  void Lock() {
    ::EnterCriticalSection(&critical_section);
  };

  bool TryLock() {
    return TryEnterCriticalSection(&critical_section) != 0;
  };

  void Unlock() {
    LeaveCriticalSection(&critical_section);
  }
};

#endif
