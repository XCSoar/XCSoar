// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

struct GeoPoint;
class AtmosphericPressure;
class GliderLinkId;
class Temperature;

/**
 * C++ wrapper for the Java interface SensorListener.
 */
class SensorListener {
public:
  virtual void OnConnected(int connected) noexcept = 0;

  /**
   * @param geoid_altitude is the GPS altitude above Geoid (true) or
   * above the WGS84 ellipsoid (false)?
   */
  virtual void OnLocationSensor(std::chrono::system_clock::time_point time,
                                int n_satellites,
                                GeoPoint location,
                                bool hasAltitude, bool geoid_altitude,
                                double altitude,
                                bool hasBearing, double bearing,
                                bool hasSpeed, double ground_speed,
                                bool hasAccuracy, double accuracy) noexcept = 0;

#ifdef ANDROID
  virtual void OnAccelerationSensor(double acceleration) noexcept = 0;
  virtual void OnAccelerationSensor(float ddx, float ddy,
                                    float ddz) noexcept = 0;
  virtual void OnRotationSensor(float dtheta_x, float dtheta_y,
                                float dtheta_z) noexcept = 0;
  virtual void OnMagneticFieldSensor(float h_x, float h_y, float h_z) noexcept = 0;
  virtual void OnBarometricPressureSensor(float pressure,
                                          float sensor_noise_variance) noexcept = 0;
  virtual void OnPressureAltitudeSensor(float altitude) noexcept = 0;
  virtual void OnI2CbaroSensor(int index, int sensorType,
                               AtmosphericPressure pressure) noexcept = 0;
  virtual void OnVarioSensor(float vario) noexcept = 0;
  virtual void OnHeartRateSensor(unsigned bpm) noexcept = 0;

  virtual void OnVoltageValues(int temp_adc, unsigned voltage_index,
                               int volt_adc) noexcept = 0;

  virtual void OnNunchukValues(int joy_x, int joy_y,
                               int acc_x, int acc_y, int acc_z,
                               int switches) noexcept = 0;

  virtual void OnGliderLinkTraffic(GliderLinkId id, const char *callsign,
                                   GeoPoint location, double altitude,
                                   double gspeed, double vspeed,
                                   unsigned bearing) noexcept = 0;

  virtual void OnTemperature(Temperature temperature) noexcept = 0;

  virtual void OnBatteryPercent(double battery_percent) noexcept = 0;

  virtual void OnSensorStateChanged() noexcept = 0;
  virtual void OnSensorError(const char *msg) noexcept = 0;
#endif // ANDROID
};
