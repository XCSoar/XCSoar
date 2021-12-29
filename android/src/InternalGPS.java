/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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

/**
 * Code to support the internal GPS receiver via #LocationManager.
 */
public class InternalGPS
  implements LocationListener, Runnable, AndroidSensor
{
  private final Handler handler;

  private final SensorListener listener;

  /** the name of the currently selected location provider */
  static final String locationProvider = LocationManager.GPS_PROVIDER;

  private final LocationManager locationManager;
  private static boolean queriedLocationSettings = false;

  private int state = STATE_LIMBO;

  private final SafeDestruct safeDestruct = new SafeDestruct();

  InternalGPS(Context context, SensorListener listener) {
    handler = new Handler(context.getMainLooper());
    this.listener = listener;

    locationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);
    if (locationManager == null ||
        locationManager.getProvider(locationProvider) == null) {
      /* on the Nook Simple Touch, LocationManager.isProviderEnabled()
         can crash, but LocationManager.getProvider() appears to be
         safe, therefore we're first checking the latter; if the
         device does have a GPS, it returns non-null even when the
         user has disabled GPS */
      return;
    } else if (!locationManager.isProviderEnabled(locationProvider) &&
        !queriedLocationSettings) {
      // Let user turn on GPS, XCSoar is not allowed to.
      Intent myIntent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
      context.startActivity(myIntent);
      queriedLocationSettings = true;
    }

    handler.post(this);
  }

  /**
   * Called by the #Handler, indirectly by update().  Updates the
   * LocationManager subscription inside the main thread.
   */
  @Override public void run() {
    try {
      locationManager.requestLocationUpdates(locationProvider,
                                             1000, 0, this);
    } catch (IllegalArgumentException e) {
      /* this exception was recorded on the Android Market, message
         was: "provider=gps" - no idea what that means */
    }
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();

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
