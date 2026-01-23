// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.os.Handler;
import android.os.Looper;
import android.os.Bundle;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.provider.Settings;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.Manifest;
import android.app.AlertDialog;
import android.util.Log;

/**
 * Code to support the internal GPS receiver via #LocationManager.
 */
public class InternalGPS
  implements LocationListener, Runnable, AndroidSensor,
  PermissionManager.PermissionHandler
{
  private static final String TAG = "InternalGPS";
  private final Context context;
  private final Handler handler;
  private final PermissionManager permissionManager;

  private final SensorListener listener;

  /** the name of the currently selected location provider */
  static final String locationProvider = LocationManager.GPS_PROVIDER;

  private final LocationManager locationManager;

  private int state = STATE_LIMBO;

  private final SafeDestruct safeDestruct = new SafeDestruct();

  /* Reference to GPS required dialog, so we can dismiss it when GPS is enabled */
  private AlertDialog gpsRequiredDialog = null;

  InternalGPS(Context context, PermissionManager permissionManager,
              SensorListener listener) {
    this.context = context;
    handler = new Handler(Looper.getMainLooper());
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
    Log.d(TAG, "run() called");
    try {
      /* Request foreground location permission first */
      boolean permissionAlreadyGranted = permissionManager.requestPermission(Manifest.permission.ACCESS_FINE_LOCATION,
                                             new PermissionManager.PermissionHandler() {
        @Override
        public void onRequestPermissionsResult(boolean granted) {
          Log.d(TAG, "Foreground location permission result: " + granted);
          if (granted) {
            /* Foreground location granted. Now request background location if needed. */
            if (android.os.Build.VERSION.SDK_INT >= 29) {
              /* Android 10+ requires separate background location permission */
              if (!permissionManager.areLocationPermissionsGranted()) {
                Log.d(TAG, "Background location not granted, requesting it");
                /* Background location not yet granted, request it */
                permissionManager.requestPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION,
                                                   new PermissionManager.PermissionHandler() {
                    @Override
                    public void onRequestPermissionsResult(boolean bgGranted) {
                      Log.d(TAG, "Background location permission result: " + bgGranted);
                      if (bgGranted) {
                        /* All location permissions granted, check GPS */
                        checkGpsAndShowDialogIfNeeded();
                      } else {
                        submitError("Background location permission denied by user");
                      }
                    }
                  });
              } else {
                Log.d(TAG, "All location permissions already granted, checking GPS");
                /* All permissions already granted, check GPS */
                checkGpsAndShowDialogIfNeeded();
              }
            } else {
              Log.d(TAG, "Android < 10, only foreground location needed, checking GPS");
              /* Android < 10, only foreground location needed */
              checkGpsAndShowDialogIfNeeded();
            }
          } else {
            submitError("Location permission denied by user");
          }
        }
      });
      Log.d(TAG, "requestPermission returned: " + permissionAlreadyGranted);
      if (permissionAlreadyGranted) {
        Log.d(TAG, "Foreground location permission already granted");
        /* Permission already granted, check if we need background location */
        if (permissionManager.areLocationPermissionsGranted()) {
          Log.d(TAG, "All location permissions already granted, checking GPS");
          /* All permissions granted, check GPS */
          checkGpsAndShowDialogIfNeeded();
        } else if (android.os.Build.VERSION.SDK_INT >= 29) {
          Log.d(TAG, "Foreground granted but background not, requesting background");
          /* Foreground granted but background not - request it */
          permissionManager.requestPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION,
                                             new PermissionManager.PermissionHandler() {
            @Override
            public void onRequestPermissionsResult(boolean bgGranted) {
              Log.d(TAG, "Background location permission result: " + bgGranted);
            if (bgGranted) {
              /* All location permissions granted, check GPS */
              checkGpsAndShowDialogIfNeeded();
            } else {
                submitError("Background location permission denied by user");
              }
            }
          });
        }
      }
    } catch (Exception e) {
      Log.e(TAG, "Exception in run(): " + e.toString(), e);
      submitError("Exception in run(): " + e.toString());
    }
  }


  @Override
  public void onRequestPermissionsResult(boolean granted) {
    /* Permission flow is handled in run() with explicit handlers */
    if (!granted)
      submitError("Permission denied by user");
  }

  /**
   * Check if GPS is enabled and show consent dialog if needed.
   * This is called after all location permissions are granted.
   * If GPS is disabled, we always show the dialog to ask the user to enable it.
   */
  private void checkGpsAndShowDialogIfNeeded() {
    Log.d(TAG, "checkGpsAndShowDialogIfNeeded() called");
    try {
      boolean isEnabled = locationManager.isProviderEnabled(locationProvider);
      Log.d(TAG, "GPS provider enabled: " + isEnabled);
      if (!isEnabled) {
        Log.d(TAG, "GPS is disabled, showing dialog");
        // Let user turn on GPS, XCSoar is not allowed to.
        // Show consent dialog explaining why GPS needs to be enabled,
        // then open Settings when user clicks OK (consistent with permission flow).
        if (context instanceof android.app.Activity) {
          final android.app.Activity activity = (android.app.Activity)context;
          handler.post(new Runnable() {
              @Override
              public void run() {
                Log.d(TAG, "Posting dialog show to main thread");
                // Check if activity is still valid before showing dialog
                if (activity.isFinishing() || (android.os.Build.VERSION.SDK_INT >= 17 && activity.isDestroyed())) {
                  Log.w(TAG, "Activity not valid, cannot show GPS dialog");
                  return;
                }
                
                Log.d(TAG, "Showing GPS required dialog");
                android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(activity);
                builder.setTitle("GPS Required");
                builder.setMessage("XCSoar needs GPS to be enabled to determine your location for flight logging and navigation. " +
                                 "Please enable GPS in system settings.");
                builder.setPositiveButton("Open Settings", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                      Log.d(TAG, "User clicked Open Settings");
                      Intent myIntent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
                      activity.startActivity(myIntent);
                    }
                  });
                builder.setNegativeButton("Cancel", null);
                builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
                    @Override
                    public void onCancel(DialogInterface dialog) {
                      Log.d(TAG, "GPS required dialog cancelled");
                      gpsRequiredDialog = null;
                    }
                  });
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                      Log.d(TAG, "GPS required dialog dismissed");
                      gpsRequiredDialog = null;
                    }
                  });
                gpsRequiredDialog = builder.show();
                Log.d(TAG, "GPS required dialog shown");
              }
            });
        } else {
          Log.w(TAG, "Context is not an Activity, using fallback");
          // Fallback if context is not an Activity (shouldn't happen, but be safe)
          Intent myIntent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
          context.startActivity(myIntent);
        }
      } else {
        Log.d(TAG, "GPS is enabled, requesting location updates");
        /* GPS is enabled, request location updates */
        try {
          locationManager.requestLocationUpdates(locationProvider,
                                                  1000, 0, this);
        } catch (SecurityException e) {
          Log.e(TAG, "SecurityException requesting location updates: " + e.toString());
          submitError(e.toString());
        }
      }
    } catch (SecurityException e) {
      Log.e(TAG, "SecurityException in checkGpsAndShowDialogIfNeeded: " + e.toString());
      submitError(e.toString());
    } catch (IllegalArgumentException e) {
      Log.e(TAG, "IllegalArgumentException in checkGpsAndShowDialogIfNeeded: " + e.toString());
      /* this exception was recorded on the Android Market, message
         was: "provider=gps" - no idea what that means */
    }
  }

  /**
   * Re-check GPS status. This should be called when the app resumes
   * (e.g., after user returns from Settings) to dismiss the dialog
   * if GPS was enabled.
   */
  public void recheckGpsStatus() {
    Log.d(TAG, "recheckGpsStatus() called");
    handler.post(new Runnable() {
        @Override
        public void run() {
          try {
            boolean isEnabled = locationManager.isProviderEnabled(locationProvider);
            Log.d(TAG, "GPS provider enabled on recheck: " + isEnabled);
            if (isEnabled) {
              /* GPS is now enabled, dismiss dialog if showing */
              if (gpsRequiredDialog != null && gpsRequiredDialog.isShowing()) {
                Log.d(TAG, "GPS is now enabled, dismissing GPS required dialog");
                gpsRequiredDialog.dismiss();
                gpsRequiredDialog = null;
              }
              /* Request location updates if not already requested */
              try {
                locationManager.requestLocationUpdates(locationProvider,
                                                        1000, 0, InternalGPS.this);
              } catch (SecurityException e) {
                Log.e(TAG, "SecurityException in recheckGpsStatus: " + e.toString());
                submitError(e.toString());
              }
            }
          } catch (Exception e) {
            Log.e(TAG, "Exception in recheckGpsStatus: " + e.toString());
          }
        }
      });
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();

    permissionManager.cancelRequestPermission(InternalGPS.this);

    handler.removeCallbacks(this);
    handler.post(new Runnable() {
        @Override public void run() {
          locationManager.removeUpdates(InternalGPS.this);
          /* Dismiss GPS dialog if showing */
          if (gpsRequiredDialog != null && gpsRequiredDialog.isShowing()) {
            gpsRequiredDialog.dismiss();
            gpsRequiredDialog = null;
          }
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
    Log.d(TAG, "onProviderEnabled() called for provider: " + provider);
    setConnectedSafe(1); // waiting for fix
    /* GPS was enabled, request location updates */
    if (provider.equals(locationProvider)) {
      Log.d(TAG, "GPS provider enabled, dismissing dialog if showing and requesting location updates");
      /* Dismiss GPS required dialog if it's showing */
      if (gpsRequiredDialog != null && gpsRequiredDialog.isShowing()) {
        Log.d(TAG, "Dismissing GPS required dialog");
        gpsRequiredDialog.dismiss();
        gpsRequiredDialog = null;
      }
      try {
        locationManager.requestLocationUpdates(locationProvider,
                                                1000, 0, this);
      } catch (SecurityException e) {
        Log.e(TAG, "SecurityException in onProviderEnabled: " + e.toString());
        submitError(e.toString());
      }
    }
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
