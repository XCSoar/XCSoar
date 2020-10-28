/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_OVERLAPPED_EVENT_HPP
#define XCSOAR_OVERLAPPED_EVENT_HPP

#include <windows.h>

class OverlappedEvent {
  OVERLAPPED os;

public:
  enum WaitResult {
    FINISHED,
    TIMEOUT,
    CANCELED,
  };

public:
  OverlappedEvent()
  {
    os.Offset = os.OffsetHigh = 0;
    os.Internal = os.InternalHigh = 0;

    os.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  }

  ~OverlappedEvent() {
    ::CloseHandle(os.hEvent);
  }

  bool Defined() const {
    return os.hEvent != nullptr;
  }

  OVERLAPPED *GetPointer() {
    return &os;
  }

  WaitResult Wait(unsigned timeout_ms=INFINITE) {
    switch (::WaitForSingleObject(os.hEvent, timeout_ms)) {
    case WAIT_OBJECT_0:
      return FINISHED;

    case WAIT_TIMEOUT:
      return TIMEOUT;

    default:
      return CANCELED;
    }
  }
};

#endif
