// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "State.hpp"

#include <optional>

/**
 * Query `org.freedesktop.NetworkManager` (Connectivity) over the system
 * D-Bus.  Returns no value if NetworkManager is missing, errors, or reports
 * unknown connectivity; callers should fall back to e.g. sysfs
 * (#PollNetStateLinuxSysfs on Linux / Kobo).
 */
[[gnu::warn_unused_result]] std::optional<NetState>
TryGetNetStateFromNetworkManager() noexcept;
