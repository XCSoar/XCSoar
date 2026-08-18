// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>
#include <string_view>

class OrderedTask;

[[gnu::pure]]
bool
HasReceivedTask() noexcept;

std::unique_ptr<OrderedTask>
GetReceivedTask() noexcept;

/**
 * Ask the UI thread to show the task that has just been stored, by
 * calling MainWindow::OnTaskReceived() from the event loop.
 *
 * This is implemented by the UI layer (MainWindow.cpp) rather than
 * here, because only that knows how to reach the main window.  It may
 * be called from any thread, and does nothing at all while the event
 * loop does not exist yet: the task stays pending and
 * MainWindow::RunTimer() offers it again once XCSoar is up.
 */
void
PostReceivedTask() noexcept;

void
ReceiveXCTrackTask(std::string_view data);

/**
 * Handle the raw text of a scanned task QR code, i.e. a payload
 * prefixed with its URI scheme such as "XCTSK:{...}".
 *
 * Unlike ReceiveXCTrackTask(), this inspects the prefix, so it can
 * grow support for further task QR flavours without the callers
 * having to know about any of them.
 *
 * Throws on an unknown prefix or a malformed payload.
 */
void
ReceiveTaskQRCode(std::string_view text);
