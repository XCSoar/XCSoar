/*
Copyright_License {

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

#include "Descriptor.hpp"
#include "DataEditor.hpp"
#include "NMEA/Info.hpp"
#include "Geo/Geoid.hpp"
#include "time/FloatDuration.hxx"

using namespace std::chrono;

void
DeviceDescriptor::OnConnected(int connected) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();

  switch (connected) {
  case 0: /* not connected */
    basic.alive.Clear();
    basic.location_available.Clear();
    break;

  case 1: /* waiting for fix */
    basic.alive.Update(basic.clock);
    basic.gps.nonexpiring_internal_gps = true;
    basic.location_available.Clear();
    break;

  case 2: /* connected */
    basic.alive.Update(basic.clock);
    basic.gps.nonexpiring_internal_gps = true;
    break;
  }

  e.Commit();
}

void
DeviceDescriptor::OnLocationSensor(std::chrono::system_clock::time_point time,
                                   int n_satellites,
                                   GeoPoint location,
                                   bool hasAltitude, bool geoid_altitude,
                                   double altitude,
                                   bool hasBearing, double bearing,
                                   bool hasSpeed, double ground_speed,
                                   bool hasAccuracy, double accuracy) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  const BrokenDateTime date_time{time};
  const BrokenDateTime midnight = date_time.AtMidnight();
  TimeStamp second_of_day{
    duration_cast<FloatDuration>(time - midnight.ToTimePoint())};

  basic.time = second_of_day;
  basic.time_available.Update(basic.clock);
  basic.date_time_utc = date_time;

  basic.gps.satellites_used = n_satellites;
  basic.gps.satellites_used_available.Update(basic.clock);
  basic.gps.real = true;
  basic.gps.nonexpiring_internal_gps = true;
  basic.location = location;
  basic.location_available.Update(basic.clock);

  if (hasAltitude) {
    auto GeoidSeparation = geoid_altitude
      ? 0.
      : EGM96::LookupSeparation(basic.location);
    basic.gps_altitude = altitude - GeoidSeparation;
    basic.gps_altitude_available.Update(basic.clock);
  } else
    basic.gps_altitude_available.Clear();

  if (hasBearing) {
    basic.track = Angle::Degrees(bearing);
    basic.track_available.Update(basic.clock);
  } else
    basic.track_available.Clear();

  if (hasSpeed) {
    basic.ground_speed = ground_speed;
    basic.ground_speed_available.Update(basic.clock);
  }

  basic.gps.hdop = hasAccuracy ? accuracy : -1;

  e.Commit();
}

#ifdef ANDROID

void
DeviceDescriptor::OnAccelerationSensor(double acceleration) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  basic.acceleration.ProvideGLoad(acceleration);

  e.Commit();
}

void
DeviceDescriptor::OnAccelerationSensor(float ddx, float ddy,
                                       float ddz) noexcept
{
  // TODO
}

void
DeviceDescriptor::OnRotationSensor(float dtheta_x, float dtheta_y,
                                   float dtheta_z) noexcept
{
  // TODO
}

void
DeviceDescriptor::OnMagneticFieldSensor(float h_x, float h_y,
                                        float h_z) noexcept
{
  // TODO
}

/**
 * Helper function for
 * Java_org_xcsoar_NonGPSSensors_setBarometricPressure: Given a
 * current measurement of the atmospheric pressure and the rate of
 * change of atmospheric pressure (in millibars and millibars per
 * second), compute the uncompensated vertical speed of the glider in
 * meters per second, assuming standard atmospheric conditions
 * (deviations from these conditions should not matter very
 * much). This calculation can be derived by taking the formula for
 * converting barometric pressure to pressure altitude (see e.g.
 * http://psas.pdx.edu/RocketScience/PressureAltitude_Derived.pdf),
 * expressing it as a function P(t), the atmospheric pressure at time
 * t, then taking the derivative with respect to t. The dP(t)/dt term
 * is the pressure change rate.
 */
gcc_pure
static inline double
ComputeNoncompVario(const double pressure, const double d_pressure)
{
  static constexpr double FACTOR(-2260.389548275485);
  static constexpr double EXPONENT(-0.8097374740609689);
  return FACTOR * pow(pressure, EXPONENT) * d_pressure;
}

void
DeviceDescriptor::OnBarometricPressureSensor(float pressure,
                                             float sensor_noise_variance) noexcept
{
  /* Kalman filter updates are also protected by the blackboard
     mutex. These should not take long; we won't hog the mutex
     unduly. */
  kalman_filter.Update(pressure, sensor_noise_variance);

  const auto e = BeginEdit();
  NMEAInfo &basic = *e;

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                kalman_filter.GetXVel()));
  basic.ProvideStaticPressure(
      AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));

  e.Commit();
}

void
DeviceDescriptor::OnPressureAltitudeSensor(float altitude) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.ProvidePressureAltitude(altitude);

  e.Commit();
}

void
DeviceDescriptor::OnI2CbaroSensor(int index, int sensorType,
                                  AtmosphericPressure pressure) noexcept
{
  if (!pressure.IsPlausible())
    return;

  DeviceConfig::PressureUse press_use;
  switch (config.port_type) {
  case DeviceConfig::PortType::I2CPRESSURESENSOR:
    assert(index == 0);
    press_use = config.press_use;
    break;

  case DeviceConfig::PortType::DROIDSOAR_V2:
    assert(index < 2);
    press_use = index == 0
      ? DeviceConfig::PressureUse::STATIC_WITH_VARIO
      : DeviceConfig::PressureUse::PITOT;
    break;

  default:
    return;
  }

  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  double param;

  // Set filter properties depending on sensor type
  if (sensorType == 85 && press_use == DeviceConfig::PressureUse::STATIC_WITH_VARIO) {
    kalman_filter.SetAccelerationVariance(KF_I2C_VAR_ACCEL_85);
    param = 0.05;
  } else {
    kalman_filter.SetAccelerationVariance(KF_I2C_VAR_ACCEL);
    param = 0.5;
  }

  kalman_filter.Update(pressure.GetHectoPascal(), param);
  const auto filtered_pressure =
    AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs());

  switch (press_use) {
  case DeviceConfig::PressureUse::NONE:
    break;

  case DeviceConfig::PressureUse::STATIC_ONLY:
    basic.ProvideStaticPressure(filtered_pressure);
    break;

  case DeviceConfig::PressureUse::STATIC_WITH_VARIO:
    basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                  kalman_filter.GetXVel()));
    basic.ProvideStaticPressure(filtered_pressure);
    break;

  case DeviceConfig::PressureUse::TEK_PRESSURE:
    basic.ProvideTotalEnergyVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                      kalman_filter.GetXVel()));
    break;

  case DeviceConfig::PressureUse::PITOT:
    basic.ProvidePitotPressure(filtered_pressure - AtmosphericPressure::HectoPascal(config.sensor_offset));
    break;
  }

  e.Commit();
}

void
DeviceDescriptor::OnVarioSensor(float vario) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.ProvideNoncompVario(vario);

  e.Commit();
}

void
DeviceDescriptor::OnHeartRateSensor(unsigned bpm) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.heart_rate = bpm;
  basic.heart_rate_available.Update(basic.clock);

  e.Commit();
}

void
DeviceDescriptor::OnVoltageValues(int temp_adc, unsigned voltage_index,
                                  int volt_adc) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  // When no calibration data present, use defaults
  if (voltage_factor == 0) {
    // Set default for temp sensor only when sensor present.
    if (temp_adc >= 0 && voltage_offset == 0)
      voltage_offset = -130;
    voltage_factor = 0.01599561738;
    basic.ProvideSensorCalibration(voltage_factor, voltage_offset);
  }

  if (temp_adc >= 0) {
    auto v = Temperature::FromCelsius(voltage_offset + temp_adc);
    if (temperature_filter.Update(v.ToNative()))
      v = Temperature::FromNative(temperature_filter.Average());
    basic.temperature = v;
    basic.temperature_available = true;
  } else {
    basic.temperature_available = false;
  }

  if (voltage_index < voltage_filter.size()) {
    auto v = voltage_factor * volt_adc;
    if (voltage_filter[voltage_index].Update(v))
      v = voltage_filter[voltage_index].Average();
    basic.voltage = v;
    basic.voltage_available.Update(basic.clock);
  }

  e.Commit();
}

void
DeviceDescriptor::OnNunchukValues(int joy_x, int joy_y,
                                  int acc_x, int acc_y, int acc_z,
                                  int switches) noexcept
{
  // Nunchuk really connected  ?
  if (joy_x < 1000) {
    {
      const auto e = BeginEdit();
      NMEAInfo &basic = *e;
      basic.UpdateClock();
      basic.acceleration.ProvideGLoad(acc_z / 1000.);
      e.Commit();
    }

    int new_joy_state_x = 0, new_joy_state_y = 0;
    if (joy_x < -50) new_joy_state_x = -1; else if (joy_x > 50) new_joy_state_x = 1;
    if (joy_y < -50) new_joy_state_y = -1; else if (joy_y > 50) new_joy_state_y = 1;

    if (new_joy_state_x && new_joy_state_x != joy_state_x) {
      if (new_joy_state_x < 0) {
        // generate event
      } else {
        // generate event
      }
    }
    joy_state_x = new_joy_state_x;

    if (new_joy_state_y && new_joy_state_y != joy_state_y) {
      if (new_joy_state_y < 0) {
        // generate event
      } else {
        // generate event
      }
    }
    joy_state_y = new_joy_state_y;
  }

  // Kludge: some IOIO digital inputs can be used without a Nunchuk.
  for (int i=0; i<8; i++) {
    if (switches & (1<<i)) {
      // generate event
    }
  }
}

void
DeviceDescriptor::OnGliderLinkTraffic(GliderLinkId id, const char *callsign,
                                      GeoPoint location, double altitude,
                                      double gspeed, double vspeed,
                                      unsigned bearing) noexcept
{
  // GliderLink uses these special values in case they don't have a real value  
  const double ALT_NONE = -10000.0;
  const double BEARING_NONE = 361.0;
  const double GSPEED_NONE = -1.0;
  const double VSPEED_NONE = -8675309.0;

  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  GliderLinkTrafficList &traffic_list = basic.glink_data.traffic;

  GliderLinkTraffic *traffic = traffic_list.FindTraffic(id);
  if (traffic == nullptr) {
    traffic = traffic_list.AllocateTraffic();
    if (traffic == nullptr)
      // no more slots available
      return;

    traffic->Clear();
    traffic->id = id;

    traffic_list.new_traffic.Update(basic.clock);
  }

  traffic->name.SetASCII(callsign);

  traffic->location = location;

  traffic->altitude_received = altitude > ALT_NONE;
  if (traffic->altitude_received)
    traffic->altitude = altitude;
  traffic->speed_received = gspeed >= GSPEED_NONE;
  if (traffic->speed_received)
    traffic->speed = gspeed;
  traffic->climb_rate_received = vspeed > VSPEED_NONE;
  if (traffic->climb_rate_received)
    traffic->climb_rate = vspeed;
  traffic->track_received = bearing < BEARING_NONE;
  if (traffic->track_received)
    traffic->track = Angle::Degrees(bearing);

  // set time of fix to current time
  traffic->valid.Update(basic.clock);

  e.Commit();
}

void
DeviceDescriptor::OnTemperature(Temperature temperature) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  basic.temperature = temperature;
  basic.temperature_available = true;

  e.Commit();
}

void
DeviceDescriptor::OnBatteryPercent(double battery_percent) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  basic.battery_level = battery_percent;
  basic.battery_level_available.Update(basic.clock);

  e.Commit();
}

void
DeviceDescriptor::OnSensorStateChanged() noexcept
{
  PortStateChanged();
}

void
DeviceDescriptor::OnSensorError(const char *msg) noexcept
{
  PortError(msg);
}

#endif
