// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbManager;
import android.util.Log;

/**
 * A BroadcastReceiver that listens for USB OTG attach/detach and
 * media mount/unmount events and forwards them to native code
 * via JNI so that the StorageManager can re-enumerate volumes.
 *
 * This receiver is registered dynamically by native code
 * (AndroidStorageHotplugMonitor).
 */
public class StorageHotplugReceiver extends BroadcastReceiver {
  private static final String TAG = "XCSoar-HotplugRcv";

  private final Context context;
  private int registrationCount;

  /**
   * Pointer to the native StorageHotplugHandler.
   * Currently unused — we call a static JNI method instead.
   */

  public StorageHotplugReceiver(Context context) {
    this.context = context;
  }

  /**
   * Register for relevant broadcast actions.
   */
  public void start() {
    if (registrationCount > 0)
      return;

    /* USB actions don't carry a data URI, but media actions require
       a "file" data-scheme filter.  Android IntentFilters match
       data-scheme across ALL actions, so we need two separate filters
       to avoid the scheme requirement blocking USB intents. */

    // Filter 1: USB OTG device attach/detach (no data scheme)
    IntentFilter usbFilter = new IntentFilter();
    usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
    usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);

    // Filter 2: Media (SD card, USB mass storage) mount/unmount
    IntentFilter mediaFilter = new IntentFilter();
    mediaFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    mediaFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    mediaFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
    mediaFilter.addAction(Intent.ACTION_MEDIA_EJECT);
    mediaFilter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
    mediaFilter.addDataScheme("file");

    /* On API 33+ we need RECEIVER_EXPORTED for system broadcasts. */
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
      context.registerReceiver(this, usbFilter, Context.RECEIVER_EXPORTED);
      context.registerReceiver(this, mediaFilter, Context.RECEIVER_EXPORTED);
    } else {
      context.registerReceiver(this, usbFilter);
      context.registerReceiver(this, mediaFilter);
    }
    registrationCount = 2;

    Log.i(TAG, "Hotplug receiver started");
  }

  /**
   * Unregister.
   */
  public void stop() {
    if (registrationCount <= 0)
      return;

    /* We registered the same receiver with two filters, so Android
       holds two registrations.  Unregister once per registration. */
    while (registrationCount > 0) {
      try {
        context.unregisterReceiver(this);
      } catch (IllegalArgumentException e) {
        // Already unregistered — stop early.
        break;
      }
      registrationCount--;
    }
    registrationCount = 0;

    Log.i(TAG, "Hotplug receiver stopped");
  }

  @Override
  public void onReceive(Context ctx, Intent intent) {
    final String action = intent.getAction();
    Log.i(TAG, "Received: " + action);

    // Notify native code about the topology change.
    // This is called on the Android main thread.
    onStorageTopologyChanged();
  }

  /**
   * JNI callback into native StorageManager.  Implemented on the
   * C++ side in the AndroidStorageHotplugMonitor translation unit.
   */
  private static native void onStorageTopologyChanged();
}
