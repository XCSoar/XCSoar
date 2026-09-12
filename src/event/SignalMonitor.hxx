// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <span>

class EventLoop;

#ifndef _WIN32

#include "util/BindMethod.hxx"

using SignalHandler = BoundMethod<void() noexcept>;

/**
 * Block the given signals in the calling thread, so they can later be
 * received by SignalMonitorRegister() instead of being handled by the
 * default disposition.
 *
 * This must be called before any other thread is created, because a
 * new thread inherits the signal mask, and a process-directed signal
 * is delivered to an arbitrary thread which does not block it.
 */
void
SignalMonitorBlock(std::span<const int> signos) noexcept;

/**
 * Initialise the signal monitor subsystem.
 *
 * Throws on error.
 */
void
SignalMonitorInit(EventLoop &loop);

/**
 * Deinitialise the signal monitor subsystem.
 */
void
SignalMonitorFinish() noexcept;

/**
 * Register a handler for the specified signal.  The handler will be
 * invoked in a safe context.
 */
void
SignalMonitorRegister(int signo, SignalHandler handler);

#else

static inline void
SignalMonitorInit(EventLoop &)
{
}

static inline void
SignalMonitorFinish() noexcept
{
}

static inline void
SignalMonitorBlock(std::span<const int>) noexcept
{
}

#endif
