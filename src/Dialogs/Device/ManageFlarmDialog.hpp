// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Device;
struct FlarmVersion;
struct FlarmHardware;
struct FlarmState;

void
ManageFlarmDialog(Device &device, const FlarmVersion &version,
                  FlarmHardware &hardware, const FlarmState &state);
