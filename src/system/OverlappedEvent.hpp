// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <handleapi.h>

class OverlappedEvent {
  OVERLAPPED os;

public:
  enum WaitResult {
    FINISHED,
    TIMEOUT,
    CANCELED,
  };

public:
  OverlappedEvent() noexcept
  {
    os.Offset = os.OffsetHigh = 0;
    os.Internal = os.InternalHigh = 0;

    os.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  }

  ~OverlappedEvent() noexcept {
    ::CloseHandle(os.hEvent);
  }

  bool Defined() const noexcept {
    return os.hEvent != nullptr;
  }

  OVERLAPPED *GetPointer() noexcept {
    return &os;
  }

  WaitResult Wait(unsigned timeout_ms=INFINITE) noexcept {
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
