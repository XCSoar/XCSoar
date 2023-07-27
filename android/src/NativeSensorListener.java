// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

/**
 * An #SensorListener implementation that passes method calls to
 * native code.
 */
final class NativeSensorListener implements SensorListener {
  /**
   * A native pointer.
   */
  private final long ptr;

  NativeSensorListener(long ptr) {
    this.ptr = ptr;
  }

  @Override
  public native void onConnected(int connected);

  @Override
  public native void onLocationSensor(long time, int n_satellites,
                                      double longitude, double latitude,
                                      boolean hasAltitude,
                                      boolean geoidAltitude,
                                      double altitude,
                                      boolean hasBearing, double bearing,
                                      boolean hasSpeed, double speed,
                                      boolean hasAccuracy, double accuracy);

  @Override
  public native void onAccelerationSensor1(double acceleration);

  @Override
  public native void onEngineSensors(boolean has_cht_temp,
                                     int cht_temp,
                                     boolean has_egt_temp,
                                     int egt_temp,
                                     boolean has_ignitions_per_second,
                                     float ignitions_per_second);

  @Override
  public native void onAccelerationSensor(float ddx, float ddy, float ddz);

  @Override
  public native void onRotationSensor(float dtheta_x, float dtheta_y,
                                      float dtheta_z);

  @Override
  public native void onMagneticFieldSensor(float h_x, float h_y, float h_z);

  @Override
  public native void onBarometricPressureSensor(float pressure,
                                                float sensor_noise_variance);

  @Override
  public native void onPressureAltitudeSensor(float altitude);

  @Override
  public native void onI2CbaroSensor(int index, int sensorType, int pressure);

  @Override
  public native void onVarioSensor(float vario);

  @Override
  public native void onHeartRateSensor(int bpm);

  @Override
  public native void onVoltageValues(int temp_adc, int voltage_index,
                                     int volt_adc);

  @Override
  public native void onNunchukValues(int joy_x, int joy_y,
                                     int acc_x, int acc_y, int acc_z,
                                     int switches);

  @Override
  public native void onGliderLinkTraffic(long gid, String callsign,
                                         double longitude, double latitude,
                                         double altitude,
                                         double gspeed, double vspeed,
                                         int bearing);

  @Override
  public native void onTemperature(double temperature_kelvin);

  @Override
  public native void onBatteryPercent(double battery_percent);

  @Override
  public native void onSensorStateChanged();

  @Override
  public native void onSensorError(String msg);
}
