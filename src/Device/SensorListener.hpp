// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

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
  virtual void OnPressureAltitudeSensor(float altitude) noexcept = 0;
  virtual void OnI2CbaroSensor(int index, int sensorType,
                               AtmosphericPressure pressure) noexcept = 0;
  virtual void OnVarioSensor(float vario) noexcept = 0;
  virtual void OnHeartRateSensor(unsigned bpm) noexcept = 0;
  /**
   * @param[in] has_cht Is the Engine Cylinder Head Temperature sensor present?
   * @param[in] cht Engine Cylinder Head Temperature.
   * @param[in] has_egt Is the Engine Exhaust Gas Temperature sensor present?
   * @param[in] egt Engine Exhaust Gas Temperature.
   * @param[in] has_ignitions_per_second Are the measured ignitions valid?
   * @param[in] ignitions_per_second Engine Ignitions Per Second, firing of the spark plug per second.
   */
  virtual void OnEngineSensors(bool has_cht,
                               Temperature cht,
                               bool has_egt,
                               Temperature egt,
                               bool has_ignitions_per_second,
                               float ignitions_per_second) noexcept = 0;

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
#if defined(ANDROID) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
  virtual void OnBarometricPressureSensor(float pressure,
                      float sensor_noise_variance) noexcept = 0;
#endif // ANDROID or iPhone
};
