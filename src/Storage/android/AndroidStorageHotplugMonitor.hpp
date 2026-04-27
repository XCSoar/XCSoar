// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageHotplugMonitor.hpp"
#include "java/Ref.hxx"

/**
 * Android hotplug monitor that detects USB OTG and removable-media
 * attach/detach events.
 *
 * On the Java side a BroadcastReceiver is registered for:
 *   - android.hardware.usb.action.USB_DEVICE_ATTACHED
 *   - android.hardware.usb.action.USB_DEVICE_DETACHED
 *   - android.intent.action.MEDIA_MOUNTED
 *   - android.intent.action.MEDIA_REMOVED
 *   - android.intent.action.MEDIA_EJECT
 *
 * When any of these fires, the Java-side callback invokes
 * onStorageTopologyChanged() on the StorageHotplugHandler via JNI.
 */
class AndroidStorageHotplugMonitor : public StorageHotplugMonitor {
  StorageHotplugHandler &handler_;

  /**
   * Java-side StorageHotplugReceiver instance.
   * Uses TrivialRef because it may or may not be set, and needs
   * explicit Set/Clear lifecycle management.
   */
  Java::TrivialRef<jobject> receiver_;

public:
  explicit AndroidStorageHotplugMonitor(StorageHotplugHandler &handler) noexcept;
  ~AndroidStorageHotplugMonitor() noexcept override;

  void Start() noexcept override;
  void Stop() noexcept override;
};
