// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Thread.hpp"
#include "event/Loop.hxx"

/**
 * A thread which runs a boost::asio::io_context.
 */
class AsioThread final : protected Thread {
  EventLoop event_loop{ThreadId::Null()};

public:
  AsioThread():Thread("asio") {}

  /**
   * Start the thread.  This method should be called after creating
   * this object.
   *
   * Throws on error.
   */
  void Start();

  /**
   * Stop the thread.  This method must be called before the
   * destructor.
   */
  void Stop();

  auto &GetEventLoop() noexcept {
    return event_loop;
  }

  operator EventLoop &() noexcept {
    return event_loop;
  }

protected:
  /* virtual methods from Thread */
  void Run() noexcept override;
};
