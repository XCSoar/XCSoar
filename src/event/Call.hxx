// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <functional>

class EventLoop;

/**
 * Call the given function in the context of the #EventLoop, and wait
 * for it to finish.
 *
 * Exceptions thrown by the given function will be rethrown.
 */
void
BlockingCall(EventLoop &loop, std::function<void()> &&f);
