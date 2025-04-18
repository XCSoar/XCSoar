// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class DeviceDescriptor;
struct Declaration;
struct Waypoint;

namespace ExternalLogger {
  void Declare(const Declaration &decl, const Waypoint *home);

  /**
   * Caller is responsible for calling DeviceDescriptor::Borrow() and
   * DeviceDescriptor::Return().
   */
  void DownloadFlightFrom(DeviceDescriptor &device);
}
