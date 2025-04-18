// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Config.hpp"

#include <tchar.h>

class DataFieldEnum;

void
FillPorts(DataFieldEnum &df, const DeviceConfig &config) noexcept;

void
UpdatePortEntry(DataFieldEnum &df, DeviceConfig::PortType type,
                const TCHAR *value, const TCHAR *name) noexcept;

void
SetPort(DataFieldEnum &df, DeviceConfig::PortType type,
        const TCHAR *value) noexcept;

void
SetBluetoothPort(DataFieldEnum &df, DeviceConfig::PortType type,
                 const TCHAR *bluetooth_mac) noexcept;

void
SetPort(DataFieldEnum &df, const DeviceConfig &config) noexcept;

[[gnu::pure]]
DeviceConfig::PortType
GetPortType(const DataFieldEnum &df) noexcept;
