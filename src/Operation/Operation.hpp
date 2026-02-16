// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  virtual void SetErrorMessage(const char *text) noexcept = 0;

  /**
   * Show a human-readable (localized) short text describing the
   * current state of the operation.
   */
  virtual void SetText(const char *text) noexcept = 0;
};

class NullOperationEnvironment : public OperationEnvironment {
public:
  /* virtual methods from class OperationEnvironment */
  bool IsCancelled() const noexcept override;
  void SetCancelHandler(std::function<void()> handler) noexcept override;
  void Sleep(std::chrono::steady_clock::duration duration) noexcept override;
  void SetErrorMessage(const char *text) noexcept override;
  void SetText(const char *text) noexcept override;
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};

class QuietOperationEnvironment : public NullOperationEnvironment {
public:
  /* virtual methods from class OperationEnvironment */
  void Sleep(std::chrono::steady_clock::duration duration) noexcept override;
};
