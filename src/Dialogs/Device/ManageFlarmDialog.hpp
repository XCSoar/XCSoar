// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Device;
struct FlarmVersion;
struct FlarmHardware;
struct FlarmFlightState;

void
ManageFlarmDialog(Device &device,
                  const FlarmVersion &version,
                  FlarmHardware &hardware,
                  const FlarmFlightState &flight_state);
