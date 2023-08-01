// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class MultipleDevices;
class DeviceBlackboard;
class MergeThread;
class GlideComputer;
class CalculationThread;

/**
 * This singleton manages components that are part of XCSoar's backend
 * (e.g. sensors and other data sources, network clients, computers,
 * loggers).
 */
struct BackendComponents {
  const std::unique_ptr<DeviceBlackboard> device_blackboard;
  std::unique_ptr<MultipleDevices> devices;
  std::unique_ptr<MergeThread> merge_thread;

  std::unique_ptr<GlideComputer> glide_computer;
  std::unique_ptr<CalculationThread> calculation_thread;

  BackendComponents() noexcept;
  ~BackendComponents() noexcept;

  BackendComponents(const BackendComponents &) = delete;
  BackendComponents &operator=(const BackendComponents &) = delete;
};
