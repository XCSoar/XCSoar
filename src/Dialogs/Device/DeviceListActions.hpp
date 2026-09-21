// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct DialogLook;
class DeviceBlackboard;
class MultipleDevices;

void
ManageSelectedDevice(DeviceBlackboard &device_blackboard,
                     MultipleDevices *devices, const DialogLook &look,
                     unsigned current) noexcept;

void
DebugSelectedDevice(MultipleDevices *devices, unsigned current) noexcept;
