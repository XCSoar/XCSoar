// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class StorageHotplugHandler;

/**
 * Register / unregister the global hotplug handler that receives JNI
 * callbacks from StorageHotplugReceiver.
 *
 * Only one handler may be active at a time.  Call with nullptr to
 * unregister.
 */
void SetAndroidStorageHotplugHandler(StorageHotplugHandler *h) noexcept;
