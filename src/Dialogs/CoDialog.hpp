// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct DialogLook;
class PluggableOperationEnvironment;
namespace UI { class SingleWindow; }
namespace Co { class InvokeTask; }

/**
 * Run the specified coroutine in the I/O thread, and show a modal
 * dialog while it is running.
 *
 * Rethrows exceptions thrown by the coroutine.
 *
 * @return true if the job has finished (may have failed), false if
 * the job was cancelled by the user
 */
bool
ShowCoDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
             const char *caption, Co::InvokeTask task,
             PluggableOperationEnvironment *env);
