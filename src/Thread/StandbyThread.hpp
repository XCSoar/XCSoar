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

#ifndef XCSOAR_THREAD_STANDBY_THREAD_HPP
#define XCSOAR_THREAD_STANDBY_THREAD_HPP

#include "Compiler.h"
#include "Thread/Thread.hpp"
#include "Thread/Mutex.hpp"
#include "Cond.hxx"

/**
 * A thread which waits for work in background.  It is similar to
 * #WorkerThread, but more light-weight and provides a mutex for
 * copying parameters into the thread.
 */
class StandbyThread : private Thread {
protected:
  /**
   * A mutex that must be locked before any of the attributes or
   * methods are acessed.
   */
  Mutex mutex;

private:
  Cond cond;

  /**
   * Is the thread alive?  Unlike Thread::IsDefined(), this one is
   * thread-safe.
   */
  bool alive = false;

  /**
   * Is work pending?  This flag gets cleared by the thread as soon as
   * it starts working.
   */
  bool pending = false;

  /**
   * Is the thread currently working, i.e. inside Tick()?
   */
  bool busy = false;

  /**
   * This flag asks the thread to stop.
   */
  bool stop = false;

public:
  explicit StandbyThread(const char *_name);

  /**
   * This destructor verifies that the thread has been stopped.
   */
  ~StandbyThread();

private:
  void TriggerCommand() {
    assert(mutex.IsLockedByCurrent());

    cond.signal();
  }

  void TriggerDone() {
    assert(mutex.IsLockedByCurrent());

    cond.signal();
  }

protected:
  using Thread::SetLowPriority;
  using Thread::SetIdlePriority;

  /**
   * Wakes up the thread to do work, calls Tick().  If the thread is
   * not already running, it is launched.  Must not be called while
   * the thread is busy.
   *
   * Caller must lock the mutex.
   */
  void Trigger();

  /**
   * Same as Trigger(), but automatically lock and unlock the mutex.
   *
   * Caller must not lock the mutex.
   */
  void LockTrigger() {
    ScopeLock protect(mutex);
    Trigger();
  }

  /**
   * Is the thread currently working (i.e. inside Tick())?
   *
   * Caller must lock the mutex.
   */
  gcc_pure
  bool IsBusy() const {
    assert(mutex.IsLockedByCurrent());

    return pending || busy;
  }

  /**
   * Was the thread asked to stop?  The Tick() implementation should
   * use this to check whether to cancel the operation.
   *
   * Caller must lock the mutex.
   */
  gcc_pure
  bool IsStopped() const {
    assert(mutex.IsLockedByCurrent());

    return stop;
  }

  /**
   * Send the "stop" command to the thread.
   *
   * Caller must lock the mutex.
   */
  void StopAsync();

  /**
   * Wait until the current job is done.
   *
   * Caller must lock the mutex.
   */
  void WaitDone();

  /**
   * Same as WaitDone(), but automatically lock and unlock the mutex.
   *
   * Caller must not lock the mutex.
   */
  void LockWaitDone() {
    ScopeLock protect(mutex);
    WaitDone();
  }

  /**
   * Wait until the thread has exited.
   *
   * Caller must lock the mutex.
   */
  void WaitStopped();

  /**
   * Stop the thread synchronously.
   *
   * Caller must lock the mutex.
   */
  void Stop() {
    assert(mutex.IsLockedByCurrent());

    StopAsync();
    WaitStopped();
  }

  void LockStop() {
    ScopeLock protect(mutex);
    Stop();
  }

  /**
   * Implement this to do the actual work.  The mutex will be locked,
   * but you should unlock it while doing real work (and re-lock it
   * before returning), or the calling thread will block.
   */
  virtual void Tick() = 0;

private:
  virtual void Run();
};

#endif
