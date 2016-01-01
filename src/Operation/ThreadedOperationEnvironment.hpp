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

#ifndef XCSOAR_THREAD_OPERATION_HPP
#define XCSOAR_THREAD_OPERATION_HPP

#include "Operation/Operation.hpp"
#include "Event/DelayedNotify.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hxx"
#include "Util/StaticString.hxx"

/**
 * This is an OperationEnvironment implementation that can be run in
 * any thread, and passes the method calls to the main thread.  Useful
 * to show a progress bar in the UI thread for a job that runs in
 * another thread.
 */
class ThreadedOperationEnvironment
  : public OperationEnvironment,
    protected DelayedNotify {
  struct Data {
    StaticString<256u> error;
    StaticString<128u> text;

    unsigned progress_range, progress_position;

    bool update_error;
    bool update_text, update_progress_range, update_progress_position;

    Data()
      :text(_T("")),
       progress_range(0u), progress_position(0u),
       update_error(false), update_text(false),
       update_progress_range(false), update_progress_position(false) {}

    void SetErrorMessage(const TCHAR *_error) {
      error = _error;
      update_error = true;
    }

    void SetText(const TCHAR *_text) {
      text = _text;
      update_text = true;
    }

    bool SetProgressRange(unsigned range) {
      if (range == progress_range)
        return false;

      progress_range = range;
      update_progress_range = true;
      return true;
    }

    bool SetProgressPosition(unsigned position) {
      if (position == progress_position)
        return false;

      progress_position = position;
      update_progress_position = true;
      return true;
    }

    void ClearUpdate() {
      update_error = false;
      update_text = false;
      update_progress_range = update_progress_position = false;
    }
  };

  OperationEnvironment &other;

  mutable Mutex mutex;
  Cond cancel_cond;
  bool cancel_flag = false;

  Data data;

public:
  explicit ThreadedOperationEnvironment(OperationEnvironment &_other);

  void Cancel() {
    const ScopeLock lock(mutex);
    if (!cancel_flag) {
      cancel_flag = true;
      cancel_cond.signal();
    }
  }

private:
  bool LockSetProgressRange(unsigned range) {
    const ScopeLock lock(mutex);
    return data.SetProgressRange(range);
  }

  bool LockSetProgressPosition(unsigned position) {
    const ScopeLock lock(mutex);
    return data.SetProgressPosition(position);
  }

  Data LockReceiveData() {
    const ScopeLock lock(mutex);
    Data new_data = data;
    data.ClearUpdate();
    return new_data;
  }

public:
  /* virtual methods from class OperationEnvironment */
  bool IsCancelled() const override;
  void Sleep(unsigned ms) override;
  void SetErrorMessage(const TCHAR *error) override;
  void SetText(const TCHAR *text) override;
  void SetProgressRange(unsigned range) override;
  void SetProgressPosition(unsigned position) override;

protected:
  /* virtual methods from class DelayedNotify */
  void OnNotification() override;
};

#endif
