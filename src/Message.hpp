// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class PluggableOperationEnvironment;

namespace Message {

void
AddMessage(const char *text, const char *data=nullptr) noexcept;

/**
 * Pin a progress pill at the HUD top.  @p key identifies the slot so
 * several loads can stack.  When @p env is set, progress callbacks
 * update that same slot.
 */
void
ShowProgress(const char *text, const void *key,
             PluggableOperationEnvironment *env=nullptr) noexcept;

void
SetProgressText(const void *key, const char *text) noexcept;

void
SetProgressRange(const void *key, unsigned range) noexcept;

void
SetProgressPosition(const void *key, unsigned position) noexcept;

void
ClearProgress(const void *key) noexcept;

} // namespace Message
