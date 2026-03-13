// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageEvents.hpp"
#include "Storage/StorageHotplugMonitor.hpp"
#include "Storage/StorageDevice.hpp"
#include "Storage/StorageMonitor.hpp"
#include "util/StaticArray.hxx"

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

/**
 * Manages storage device enumeration, hotplug detection, and
 * semantic event dispatch.
 *
 * This class is UI-agnostic.  The caller provides a callback
 * (#notify_callback) that is invoked from a background thread
 * when the device list may have changed.  The callback must
 * arrange for ProcessPendingChanges() to be called on the
 * main/UI thread.
 *
 * Listeners registered via AddEventListener() receive semantic
 * StorageEventInfo notifications (available / removed / …) on
 * the main/UI thread inside ProcessPendingChanges().
 */
class StorageManager : public StorageHotplugHandler {
public:
  using NotifyCallback = std::function<void()>;

  explicit StorageManager(NotifyCallback notify_callback);
  ~StorageManager();

  void StartMonitoring() noexcept;
  void StopMonitoring() noexcept;

  /**
   * Re-enumerate devices immediately on the calling thread.  Returns
   * true if the device list has changed since the last enumeration.
   * Must be called on the main/UI thread.
   */
  bool RefreshDevices();

  struct StorageChange {
    std::vector<std::shared_ptr<StorageDevice>> added;
    std::vector<std::shared_ptr<StorageDevice>> removed;
    std::vector<std::shared_ptr<StorageDevice>> access_granted;
  };

  /**
   * Return the most recent change set.
   * Must be called on the main/UI thread.
   */
  [[nodiscard]] const StorageChange &GetLastChange() const noexcept;

  /**
   * Register a listener for semantic storage events.
   * Must be called on the main/UI thread.
   */
  void AddEventListener(StorageEventListener &listener) noexcept;

  /**
   * Unregister a previously registered listener.
   * Must be called on the main/UI thread.
   */
  void RemoveEventListener(StorageEventListener &listener) noexcept;

  [[nodiscard]] std::vector<std::shared_ptr<StorageDevice>> GetDevices() const noexcept;

  /**
   * Process pending changes produced by the background refresh
   * thread, compute semantic events, and notify listeners.
   * Must be called on the main/UI thread (typically from the
   * owner's UI::Notify callback).
   */
  void ProcessPendingChanges() noexcept;

private:
  static constexpr unsigned MAX_EVENT_LISTENERS = 4;

  NotifyCallback notify_callback_;

  std::unique_ptr<StorageHotplugMonitor> monitor_;
  std::unique_ptr<StorageMonitor> enumerator_;
  std::vector<std::shared_ptr<StorageDevice>> devices_;
  StaticArray<StorageEventListener *, MAX_EVENT_LISTENERS> event_listeners_;
  mutable std::mutex devices_mutex_;

#ifndef NDEBUG
  bool calling_listeners_ = false;
#endif

  void OnStorageTopologyChanged() noexcept override;

  void DispatchEvents(const StorageChange &change) noexcept;

  bool ApplyEnumeratedDevices(
    std::vector<std::shared_ptr<StorageDevice>> new_devices,
    StorageChange &change);

  StorageChange last_change_;

  // Async refresh control
  std::atomic<bool> shutting_down_{false};
  std::atomic<bool> refresh_running_{false};
  std::thread refresh_thread_;
  std::mutex refresh_mutex_;
  StorageChange pending_change_;
};
