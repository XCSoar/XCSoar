/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

// TODO items
// Wishlist:
// - Support more than one sensor of a particular type. Right now, only
//   the default sensor of any given type is used. This is probably OK for
//   all consumer devices.
// - Explore new and exotic sensors.

package org.xcsoar;

import android.content.Context;
import android.os.Handler;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;
import android.os.SystemClock;

/**
 * Code to support the growing suite of non-GPS sensors on Android platforms.
 */
public class NonGPSSensors implements SensorEventListener, Runnable {
  private static final String TAG = "XCSoar";

  // Constant array saying whether we want to support certain sensors.
  // If modifying this array, make certain that the largest ID value inside
  // is still less than SENSOR_TYPE_ID_UPPER_BOUND, and don't forget to add
  // corresponding constants to NonGPSSensors.(cpp|hpp).
  private static final int[] SUPPORTED_SENSORS = {
    Sensor.TYPE_ACCELEROMETER,
    Sensor.TYPE_GYROSCOPE,
    Sensor.TYPE_MAGNETIC_FIELD,
    Sensor.TYPE_PRESSURE,
  // TODO: Maybe add the following sensors if we can find a use for them.
    // Sensor.TYPE_LIGHT,
    // Sensor.TYPE_PROXIMITY,
  // TODO: For Android 4.0 and up.
    // Sensor.TYPE_AMBIENT_TEMPERATURE,
    // Sensor.TYPE_RELATIVE_HUMIDITY,
  // Remaining sensor types seem to be for "sensors" whose values are
  // derived from the above sensors in software. We're probably better off
  // taking care of that on our own.
  };

  // Non-inclusive upper bound on the largest sensor numerical type ID. As of
  // API 14, the largest sensor numerical type ID appears to be 13. Used only
  // for proportioning the following two arrays and for index checking.
  private static final int SENSOR_TYPE_ID_UPPER_BOUND = 20;

  // Constants for the Kalman filter's noise models. These values are bigger
  // than actual noise recovered from data, but that's because that noise
  // looks more Laplacian than anything.
  private static final double KF_VAR_ACCEL = 0.0075;  // Variance of pressure acceleration noise input.
  private static final double KF_VAR_MEASUREMENT = 0.05;  // Variance of pressure measurement noise.

  // The set of sensors in SUPPORTED_SENSORS that are present on this device,
  // i.e. that are retrieved by calling getDefaultSensor on sensor IDs present
  // in that array. This array is indexed by sensor numerical type ID---if the
  // corresponding sensor is absent or unsupported, the value will be null.
  private Sensor[] default_sensors_;

  // The set of sensors in SUPPORTED_SENSORS that are present on this device
  // that we are actively listening to and passing into XCSoar. This array is
  // indexed by sensor numerical type ID---if the corresponding sensor is
  // absent or unsupported, the value will be null.
  private boolean[] enabled_sensors_;

  /**
   * A Kalman filter used to smoothen the pressure sensor readings.
   */
  private KalmanFilter pressureFilter = new KalmanFilter(KF_VAR_ACCEL);

  /**
   * Time stamp of the last pressure reading.  This is used to
   * calculate delta-time for the KalmanFilter.
   */
  private double lastPressureTime = -1;

  // Sensor manager.
  private SensorManager sensor_manager_;

  // Handler for non-GPS sensor reading.
  private static Handler handler_;

  private final SafeDestruct safeDestruct = new SafeDestruct();

  /**
   * Index of this device in the global list. This value is extracted directly
   * from this object by the C++ wrapper code.
   */
  private final int index;

  /**
   * Global initialization of the class.  Must be called from the main
   * event thread, because the Handler object must be bound to that
   * thread.
   */
  public static void Initialize() {
    handler_ = new Handler();
  }

  NonGPSSensors(Context context, int _index) {
    index = _index;
    default_sensors_ = new Sensor[SENSOR_TYPE_ID_UPPER_BOUND];
    enabled_sensors_ = new boolean[SENSOR_TYPE_ID_UPPER_BOUND];

    // Obtain sensor manager.
    sensor_manager_ = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);

    // Obtain the default Sensor objects for each of the desired sensors, if
    // possible. Missing sensors will yield null values.
    for (int id : SUPPORTED_SENSORS) {
      default_sensors_[id] = sensor_manager_.getDefaultSensor(id);
    }

    updateSensorSubscriptions();
  }

  /**
   * Trigger an update to this object's sensor subscriptions. This causes
   * run() to be executed in the main thread. Should be called after each
   * change to enabled_sensors_.
   */
  private void updateSensorSubscriptions() {
    Log.d(TAG, "Triggering non-GPS sensor subscription update...");
    handler_.removeCallbacks(this);
    handler_.post(this);
  }

  /**
   * Retrieve an array of the type IDs of subscribable sensors. These are
   * the sensors that are both supported by XCSoar and present in this device.
   */
  public int[] getSubscribableSensors() {
    int[] subscribable = new int[SUPPORTED_SENSORS.length];
    int s_ind = 0;
    for (int id : SUPPORTED_SENSORS) {
      if (default_sensors_[id] != null) subscribable[s_ind++] = id;
    }
    int[] result = new int[s_ind];
    System.arraycopy(subscribable, 0, result, 0, s_ind);
    return result;
  }

  /**
   * Enable the sensor designated by the furnished type ID. Returns true if
   * the sensor is subscribable, regardless of whether it's already enabled;
   * false otherwise. It's OK (albeit dumb) to call this on an already enabled
   * sensor.
   */
  public boolean subscribeToSensor(int id) {
    if (id < 0 || id >= SENSOR_TYPE_ID_UPPER_BOUND || default_sensors_[id] == null) {
      return false;
    }
    enabled_sensors_[id] = true;
    updateSensorSubscriptions();
    return true;
  }

  /**
   * Disable the sensor designated by the furnished type ID. Returns true if
   * the sensor is subscribable, regardless of whether it's already enabled;
   * false otherwise. It's OK (albeit dumb) to call this on an already disabled
   * sensor.
   */
  public boolean cancelSensorSubscription(int id) {
    if (id < 0 || id >= SENSOR_TYPE_ID_UPPER_BOUND || default_sensors_[id] == null) {
      return false;
    }
    enabled_sensors_[id] = false;
    updateSensorSubscriptions();
    return true;
  }

  /**
   * Return true iff we are subscribed to a particular sensor.
   */
  public boolean subscribedToSensor(int id) {
    if (id < 0 || id >= SENSOR_TYPE_ID_UPPER_BOUND || default_sensors_[id] == null) {
      return false;
    }
    return enabled_sensors_[id];
  }

  /** Cancel all sensor subscriptions. */
  public void cancelAllSensorSubscriptions() {
    safeDestruct.beginShutdown();
    for (int id : SUPPORTED_SENSORS) enabled_sensors_[id] = false;
    updateSensorSubscriptions();
    safeDestruct.finishShutdown();
  }

  /**
   * from runnable; called by the #Handler and indirectly by
   * updateSensorSubscriptions(). Updates the sensor subscriptions inside the
   * main thread.
   */
  @Override
  public void run() {
    // Clear out all sensor listening subscriptions and subscribe (all over
    // again) to all desired sensors.
    Log.d(TAG, "Updating non-GPS sensor subscriptions...");
    sensor_manager_.unregisterListener(this);

    /* schedule a KalmanFilter reset */
    lastPressureTime = -1;

    for (int id : SUPPORTED_SENSORS) {
      if (enabled_sensors_[id] && default_sensors_[id] != null) {
        Log.d(TAG, "Subscribing to sensor ID " + id + " (" + default_sensors_[id].getName() + ")");
        sensor_manager_.registerListener(this, default_sensors_[id],
                                         sensor_manager_.SENSOR_DELAY_NORMAL);
      }
    }
    Log.d(TAG, "Done updating non-GPS sensor subscriptions...");
  }

  private void onBarometricPressure(float value) {
    final double now = SystemClock.elapsedRealtime() / 1000.;
    if (lastPressureTime < 0)
      pressureFilter.reset(value);
    else
      pressureFilter.update(value, KF_VAR_MEASUREMENT,
                            now - lastPressureTime);
    lastPressureTime = now;

    setBarometricPressure((float)pressureFilter.getXAbs());
  }

  /** from SensorEventListener; currently does nothing. */
  public void onAccuracyChanged(Sensor sensor, int accuracy) {
  }

  /** from SensorEventListener; report new sensor values to XCSoar. */
  public void onSensorChanged(SensorEvent event) {
    if (!safeDestruct.Increment())
      return;

    try {
      switch (event.sensor.getType()) {
      case Sensor.TYPE_ACCELEROMETER:
        setAcceleration(event.values[0], event.values[1], event.values[2]);
        break;
      case Sensor.TYPE_GYROSCOPE:
        setRotation(event.values[0], event.values[1], event.values[2]);
        break;
      case Sensor.TYPE_MAGNETIC_FIELD:
        setMagneticField(event.values[0], event.values[1], event.values[2]);
        break;
      case Sensor.TYPE_PRESSURE:
        onBarometricPressure(event.values[0]);
        break;
      }
    } finally {
      safeDestruct.Decrement();
    }
  }

  // Native methods for reporting sensor values to XCSoar's native C++ code.
  private native void setAcceleration(float ddx, float ddy, float ddz);
  private native void setRotation(float dtheta_x, float dtheta_y, float dtheta_z);
  private native void setMagneticField(float h_x, float h_y, float h_z);
  private native void setBarometricPressure(float pressure);
}
