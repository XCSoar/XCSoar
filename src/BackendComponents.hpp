// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

struct PolarSettings;
class Logger;
class NMEALogger;
class GlueFlightLogger;
class MultipleDevices;
class DeviceBlackboard;
class MergeThread;
class ProtectedTaskManager;
class ProtectedAirspaceWarningManager;
class GlideComputer;
class CalculationThread;
class Replay;

/**
 * This singleton manages components that are part of XCSoar's backend
 * (e.g. sensors and other data sources, network clients, computers,
 * loggers).
 */
struct BackendComponents {
  std::unique_ptr<Logger> igc_logger;
  std::unique_ptr<NMEALogger> nmea_logger;
  std::unique_ptr<GlueFlightLogger> flight_logger;

  const std::unique_ptr<DeviceBlackboard> device_blackboard;
  std::unique_ptr<MultipleDevices> devices;
  std::unique_ptr<MergeThread> merge_thread;

  std::unique_ptr<ProtectedTaskManager> protected_task_manager;
  std::unique_ptr<GlideComputer> glide_computer;
  std::unique_ptr<CalculationThread> calculation_thread;

  std::unique_ptr<Replay> replay;

  BackendComponents() noexcept;
  ~BackendComponents() noexcept;

  BackendComponents(const BackendComponents &) = delete;
  BackendComponents &operator=(const BackendComponents &) = delete;

/**
 * Returns the global ProtectedAirspaceWarningManager instance.  May
 * be nullptr if disabled.
 */
  [[gnu::pure]]
  ProtectedAirspaceWarningManager *GetAirspaceWarnings() noexcept;

  /**
   * Call this after modifying the #GlidePolar in #PolarSettings to
   * propagate it to the #CalculationThread and the
   * #ProtectedTaskManager.
   */
  void SetTaskPolar(const PolarSettings &settings) noexcept;
};
