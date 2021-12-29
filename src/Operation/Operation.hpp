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

#ifndef XCSOAR_OPERATION_HPP
#define XCSOAR_OPERATION_HPP

#include "ProgressListener.hpp"
#include "util/NonCopyable.hpp"

#include <chrono>
#include <exception>
#include <functional>

#include <tchar.h>

/**
 * An environment a complex operation runs in.  The operation may run
 * in a separate thread, and this class provides a bridge to the
 * calling thread.
 */
class OperationEnvironment : private NonCopyable, public ProgressListener {
public:
  void SetError(std::exception_ptr e) noexcept;

  /**
   * Has the caller requested to cancel the operation?
   */
  [[gnu::pure]]
  virtual bool IsCancelled() const noexcept = 0;

  /**
   * The caller wants a callback to be invoked when cancellation is
   * requested.  The callback must be thread-safe.  All
   * implementations supporting IsCancelled() must support this.
   */
  virtual void SetCancelHandler(std::function<void()> handler) noexcept = 0;

  /**
   * Sleep for a fixed amount of time.  May return earlier if an event
   * occurs.
   */
  virtual void Sleep(std::chrono::steady_clock::duration duration) noexcept = 0;

  /**
   * Show a human-readable (localized) short text describing the
   * error condition.
   */
  virtual void SetErrorMessage(const TCHAR *text) noexcept = 0;

  /**
   * Show a human-readable (localized) short text describing the
   * current state of the operation.
   */
  virtual void SetText(const TCHAR *text) noexcept = 0;
};

class NullOperationEnvironment : public OperationEnvironment {
public:
  /* virtual methods from class OperationEnvironment */
  bool IsCancelled() const noexcept override;
  void SetCancelHandler(std::function<void()> handler) noexcept override;
  void Sleep(std::chrono::steady_clock::duration duration) noexcept override;
  void SetErrorMessage(const TCHAR *text) noexcept override;
  void SetText(const TCHAR *text) noexcept override;
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};

class QuietOperationEnvironment : public NullOperationEnvironment {
public:
  /* virtual methods from class OperationEnvironment */
  void Sleep(std::chrono::steady_clock::duration duration) noexcept override;
};

#endif
