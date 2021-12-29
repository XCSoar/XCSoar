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

/**
 * Handler for sensor values obtained from Java code.
 */
public interface SensorListener {
  void onConnected(int connected);

  /**
   * @param geoidAltitude is the GPS altitude above Geoid (true) or
   * above the WGS84 ellipsoid (false)?
   */
  void onLocationSensor(long time, int n_satellites,
                        double longitude, double latitude,
                        boolean hasAltitude, boolean geoidAltitude,
                        double altitude,
                        boolean hasBearing, double bearing,
                        boolean hasSpeed, double speed,
                        boolean hasAccuracy, double accuracy);

  void onAccelerationSensor1(double acceleration);
  void onAccelerationSensor(float ddx, float ddy, float ddz);
  void onRotationSensor(float dtheta_x, float dtheta_y, float dtheta_z);
  void onMagneticFieldSensor(float h_x, float h_y, float h_z);
  void onBarometricPressureSensor(float pressure, float sensor_noise_variance);
  void onPressureAltitudeSensor(float altitude);
  void onI2CbaroSensor(int index, int sensorType, int pressure);
  void onVarioSensor(float vario);
  void onHeartRateSensor(int bpm);

  void onVoltageValues(int temp_adc, int voltage_index, int volt_adc);

  void onNunchukValues(int joy_x, int joy_y,
                       int accel_x, int accel_y, int accel_z,
                       int switches);

  void onGliderLinkTraffic(long gid, String callsign,
                           double longitude, double latitude, double altitude,
                           double gspeed, double vspeed,
                           double bearing);

  void onTemperature(double temperature_kelvin);

  void onBatteryPercent(double battery_percent);

  /**
   * The state has changed, and AndroidSensor.getState() will provide
   * the new value.
   */
  void onSensorStateChanged();

  /**
   * An error has occurred, and the sensor is now defunct.
   *
   * @param msg a human-readable error message
   */
  void onSensorError(String msg);
}
