// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * JNI native implementation for
 * org.xcsoar.StorageHotplugReceiver.onStorageTopologyChanged()
 * and org.xcsoar.XCSoar.onSAFPermissionGranted().
 *
 * When the Java BroadcastReceiver fires, it calls the static native
 * method.  We forward the call to the currently registered
 * StorageHotplugHandler (if any).
 *
 * Similarly, when SAF permission is granted via the document picker,
 * the XCSoar activity calls onSAFPermissionGranted() which also
 * triggers a topology change so volumes are re-enumerated.
 *
 * The handler pointer is set/cleared by the Android platform factory
 * when creating/destroying the hotplug monitor.
 */

#include "Storage/StorageHotplugMonitor.hpp"
#include "util/Compiler.h"

#include <jni.h>
#include <atomic>

/**
 * Global pointer to the active handler.
 * Written by AndroidStorageHotplugMonitor::Start/Stop (main thread),
 * read by the JNI callback (Android main/Binder thread).
 * std::atomic is needed because writer and reader may be on
 * different threads.
 */
static std::atomic<StorageHotplugHandler *> global_hotplug_handler{nullptr};

void
SetAndroidStorageHotplugHandler(StorageHotplugHandler *h) noexcept
{
  global_hotplug_handler.store(h);
}

extern "C"
gcc_visibility_default
void
Java_org_xcsoar_StorageHotplugReceiver_onStorageTopologyChanged(
    [[maybe_unused]] JNIEnv *env,
    [[maybe_unused]] jclass cls) noexcept
{
  auto *handler = global_hotplug_handler.load();
  if (handler != nullptr)
    handler->OnStorageTopologyChanged();
}

extern "C"
gcc_visibility_default
void
Java_org_xcsoar_XCSoar_onSAFPermissionGranted(
    [[maybe_unused]] JNIEnv *env,
    [[maybe_unused]] jclass cls,
    [[maybe_unused]] jstring treeUri) noexcept
{
  /* Treat a new SAF permission grant as a topology change so the
     StorageManager re-enumerates volumes and the newly accessible
     volume appears with IsWritable() == true. */
  auto *handler = global_hotplug_handler.load();
  if (handler != nullptr)
    handler->OnStorageTopologyChanged();
}
