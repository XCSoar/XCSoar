#ifndef TRIGGER_HXX
#define TRIGGER_HXX

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * This class wraps an OS specific trigger.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Trigger {
  HANDLE handle;

public:
  /**
   * Initializes the trigger.
   *
   * @param name an application specific name for this trigger
   */
  Trigger(LPCTSTR name)
    :handle(::CreateEvent(NULL, true, false, name)) {}
  ~Trigger() {
    ::CloseHandle(handle);
  }

public:
  /**
   * Waits until this object is triggered with trigger().  If this
   * object is already triggered, this method returns immediately.
   *
   * @param timeout_ms the maximum number of milliseconds to wait
   * @return true if this object was triggered, false if the timeout
   * has expired
   */
  bool wait(unsigned timeout_ms) {
    if (::WaitForSingleObject(handle, timeout_ms) != WAIT_OBJECT_0)
      return false;
    ::ResetEvent(handle);
    return true;
  }

  /**
   * Waits indefinitely until this object is triggered with trigger().
   * If this object is already triggered, this method returns
   * immediately.
   */
  void wait() {
    wait(INFINITE);
  }

  /**
   * Wakes up the thread waiting for the trigger.  The state of the
   * trigger is reset only if a thread was really woken up.
   */
  void trigger() {
    ::SetEvent(handle);
  }

  /**
   * Wakes up the thread waiting for the trigger, and immediately
   * resets the state of the trigger.
   */
  void pulse() {
    ::PulseEvent(handle);
  }
};

#endif
