// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "State.hpp"

#include <optional>

/**
 * Query ConnMan @c net.connman global @c net.connman.Manager (State) over
 * the system D-Bus.  Returns no value if ConnMan is missing, on errors, or
 * the state is not mapped; callers should fall back to sysfs
 * (#PollNetStateLinuxSysfs).
 */
[[gnu::warn_unused_result]] std::optional<NetState>
TryGetNetStateFromConnMan() noexcept;
