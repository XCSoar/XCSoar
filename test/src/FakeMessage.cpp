// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Message.hpp"

// Forward declarations for dialog functions - avoid pulling in UI dependencies
class Device;
struct DeviceInfo;
struct FlarmVersion;
struct FlarmHardware;
struct FlarmFlightState;

void ManageFlarmDialog(Device &device,
                       const FlarmVersion &version,
                       FlarmHardware &hardware,
                       const FlarmFlightState &flight_state);
void ManageCAI302Dialog(Device &device);
void ManageLXNAVVarioDialog(Device &device, const DeviceInfo &info1,
                            const DeviceInfo &info2);
void ManageLX16xxDialog(Device &device, const DeviceInfo &info);
void ManageNanoDialog(Device &device, const DeviceInfo &info);
bool dlgConfigurationVarioShowModal(Device &device);
void dlgConfigurationBlueFlyVarioShowModal(Device &device);
void ManageStratuxDialog(Device &device);

#include <stdio.h>

void
Message::AddMessage(const TCHAR *text,
                    [[maybe_unused]] const TCHAR *data) noexcept
{
  _ftprintf(stderr, _T("%s\n"), text);
}

// Stub implementations for device Manage dialog functions
// These are only needed to satisfy the linker - TestDriver never calls Manage()
// so these should never actually be invoked
void
ManageFlarmDialog([[maybe_unused]] Device &device,
                  [[maybe_unused]] const FlarmVersion &version,
                  [[maybe_unused]] FlarmHardware &hardware,
                  [[maybe_unused]] const FlarmFlightState &flight_state)
{
  // No-op in test builds
}

void
ManageCAI302Dialog([[maybe_unused]] Device &device)
{
  // No-op in test builds
}

void
ManageLXNAVVarioDialog([[maybe_unused]] Device &device,
                        [[maybe_unused]] const DeviceInfo &info1,
                        [[maybe_unused]] const DeviceInfo &info2)
{
  // No-op in test builds
}

void
ManageLX16xxDialog([[maybe_unused]] Device &device,
                   [[maybe_unused]] const DeviceInfo &info)
{
  // No-op in test builds
}

void
ManageNanoDialog([[maybe_unused]] Device &device,
                 [[maybe_unused]] const DeviceInfo &info)
{
  // No-op in test builds
}

bool
dlgConfigurationVarioShowModal([[maybe_unused]] Device &device)
{
  // No-op in test builds
  return false;
}

void
dlgConfigurationBlueFlyVarioShowModal([[maybe_unused]] Device &device)
{
  // No-op in test builds
}

void
ManageStratuxDialog([[maybe_unused]] Device &device)
{
  // No-op in test builds
}
