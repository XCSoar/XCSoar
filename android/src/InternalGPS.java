// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.os.Handler;
import android.os.Bundle;
import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.Manifest;

/**
 * Code to support the internal GPS receiver via #LocationManager.
 */
public class InternalGPS
  implements LocationListener, Runnable, AndroidSensor,
  PermissionManager.PermissionHandler
{
  private final Context context;
  private final Handler handler;
  private final PermissionManager permissionManager;

  private final SensorListener listener;

  /** the name of the currently selected location provider */
  static final String locationProvider = LocationManager.GPS_PROVIDER;

  private final LocationManager locationManager;
  private static boolean queriedLocationSettings = false;

  private int state = STATE_LIMBO;

  private final SafeDestruct safeDestruct = new SafeDestruct();

  InternalGPS(Context context, PermissionManager permissionManager,
              SensorListener listener) {
    this.context = context;
    handler = new Handler(context.getMainLooper());
    this.permissionManager = permissionManager;
    this.listener = listener;

    locationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);
    if (locationManager == null)
      /* can this really happen? */
      throw new IllegalStateException("No LocationManager");

    // schedule a run() call in the MainLooper thread
    handler.post(this);
  }

  /**
   * Called by the #Handler, indirectly by update().  Updates the
   * LocationManager subscription inside the main thread.
   */
  @Override public void run() {
    if (!permissionManager.requestPermission(Manifest.permission.ACCESS_FINE_LOCATION,
                                             this))
      /* permission is requested asynchronously,
         onRequestPermissionsResult() will be called later */
      return;

    if (android.os.Build.VERSION.SDK_INT >= 29)
      permissionManager.requestPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION,
                                          null);

    try {
      if (!locationManager.isProviderEnabled(locationProvider) &&
          !queriedLocationSettings) {
        // Let user turn on GPS, XCSoar is not allowed to.
        Intent myIntent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        context.startActivity(myIntent);
        queriedLocationSettings = true;
      }

      locationManager.requestLocationUpdates(locationProvider,
                                             1000, 0, this);
    } catch (SecurityException e) {
      submitError(e.toString());
    } catch (IllegalArgumentException e) {
      /* this exception was recorded on the Android Market, message
         was: "provider=gps" - no idea what that means */
    }
  }

  @Override
  public void onRequestPermissionsResult(boolean granted) {
    if (granted)
      /* try again */
      handler.post(this);
    else
      submitError("Permission denied by user");
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();

    permissionManager.cancelRequestPermission(InternalGPS.this);

    handler.removeCallbacks(this);
    handler.post(new Runnable() {
        @Override public void run() {
          locationManager.removeUpdates(InternalGPS.this);
        }
      });

    safeDestruct.finishShutdown();
  }

  @Override
  public int getState() {
    return state;
  }

  private void setStateSafe(int _state) {
    if (_state == state)
      return;

    state = _state;

    if (safeDestruct.increment()) {
      try {
        listener.onSensorStateChanged();
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  private void submitError(String msg) {
    state = STATE_FAILED;

    if (safeDestruct.increment()) {
      try {
        listener.onSensorError(msg);
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  private void sendLocation(Location location) {
    Bundle extras = location.getExtras();

    listener.onLocationSensor(location.getTime(),
                              extras != null ? extras.getInt("satellites", -1) : -1,
                              location.getLongitude(), location.getLatitude(),
                              location.hasAltitude(), false,
                              location.getAltitude(),
                              location.hasBearing(), location.getBearing(),
                              location.hasSpeed(), location.getSpeed(),
                              location.hasAccuracy(), location.getAccuracy());
  }

  private void setConnectedSafe(int connected) {
    if (!safeDestruct.increment())
      return;

    try {
      listener.onConnected(connected);
    } finally {
      safeDestruct.decrement();
    }
  }

  /** from LocationListener */
  @Override public void onLocationChanged(Location newLocation) {
    if (!safeDestruct.increment())
      return;

    try {
      listener.onConnected(2); // fix found
      sendLocation(newLocation);
    } finally {
      safeDestruct.decrement();
    }
  }

  /** from LocationListener */
  @Override public void onProviderDisabled(String provider) {
    setConnectedSafe(0); // not connected
  }

  /** from LocationListener */
  @Override public void onProviderEnabled(String provider) {
    setConnectedSafe(1); // waiting for fix
  }

  /** from LocationListener */
  @Override public void onStatusChanged(String provider, int status,
                                        Bundle extras) {
    if (!safeDestruct.increment())
      return;

    try {
      switch (status) {
      case LocationProvider.OUT_OF_SERVICE:
        setConnectedSafe(0); // not connected
        setStateSafe(STATE_FAILED);
        break;

      case LocationProvider.TEMPORARILY_UNAVAILABLE:
        setConnectedSafe(1); // waiting for fix
        setStateSafe(STATE_LIMBO);
        break;

      case LocationProvider.AVAILABLE:
        listener.onSensorStateChanged();
        setStateSafe(STATE_READY);
        break;
      }
    } finally {
      safeDestruct.decrement();
    }
  }
}
