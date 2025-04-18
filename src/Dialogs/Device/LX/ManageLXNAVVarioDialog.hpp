// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Device;
struct DeviceInfo;

void
ManageLXNAVVarioDialog(Device &device, const DeviceInfo &info,
               const DeviceInfo &secondary_info);
