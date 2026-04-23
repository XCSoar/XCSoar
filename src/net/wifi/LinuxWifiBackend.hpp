// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiBackend.hpp"

enum class LinuxWifiBackendKind {
  None,
  NetworkManager,
  ConnMan,
};

LinuxWifiBackendKind
QueryLinuxWifiBackendKind();

[[gnu::pure]]
bool
HasLinuxWifiRadioToggle(LinuxWifiBackendKind backend_kind) noexcept;

bool
GetLinuxWifiRadioEnabled(LinuxWifiBackendKind backend_kind);

void
SetLinuxWifiRadioEnabled(LinuxWifiBackendKind backend_kind, bool enabled);

UniqueWifiBackend
CreateLinuxWifiBackend(LinuxWifiBackendKind backend_kind);

UniqueWifiBackend
CreateLinuxWifiBackend();