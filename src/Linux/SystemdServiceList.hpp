// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>
#include <vector>

/**
 * One explicitly selected systemd unit exposed by XCSoar.
 *
 * Unit names are deliberately not supplied by users: this list is an
 * allowlist of host facilities which XCSoar is permitted to control.
 */
struct SystemdService {
  const char *id;
  const char *display_name;
  const char *description;
  std::string unit_name;
};

/**
 * Resolve the selected unit allowlist against the local system.
 * Only installed units are returned.  For facilities with distro-specific
 * names (such as SSH), the active alternative is preferred, followed by an
 * enabled alternative and then any installed alternative.  Ties retain the
 * configured list order.
 */
std::vector<SystemdService>
BuildSystemdServiceList() noexcept;
