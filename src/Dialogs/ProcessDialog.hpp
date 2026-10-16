// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <functional>

struct DialogLook;
namespace UI { class SingleWindow; }

/**
 * Run a process with live output.  If @p progress_text is set, show it until
 * the process produces output and append Complete/Failed when it exits.
 * Call @p on_exit with the exit status (EXIT_FAILURE if startup fails).
 * A nonzero callback result closes the dialog with that modal result;
 * otherwise the output remains visible until the user closes it.
 * Cancellation sends SIGTERM to the process group, allows ten seconds for
 * cleanup before SIGKILL, and closes without calling @p on_exit.
 * Output is drained until EOF or that deadline.  A child still running at
 * the deadline is reaped asynchronously after the dialog closes.
 */
int
RunProcessDialog(UI::SingleWindow &parent,
                 const DialogLook &dialog_look,
                 const char *caption,
                 const char *const*argv,
                 std::function<int(int)> on_exit = {},
                 const char *progress_text = nullptr) noexcept;
