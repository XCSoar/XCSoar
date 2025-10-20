// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Descriptor.hpp"
#include "SmartDeviceSensors.hpp"
#include "DataEditor.hpp"
#include "NMEA/Info.hpp"
#include "time/FloatDuration.hxx"

using namespace std::chrono;

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
DeviceDescriptor::OnAccelerationSensor([[maybe_unused]] float ddx, [[maybe_unused]] float ddy,
                                       [[maybe_unused]] float ddz) noexcept
{
  // TODO
}

void
DeviceDescriptor::OnRotationSensor([[maybe_unused]] float dtheta_x, [[maybe_unused]] float dtheta_y,
                                   [[maybe_unused]] float dtheta_z) noexcept
{
  // TODO
}

void
DeviceDescriptor::OnMagneticFieldSensor([[maybe_unused]] float h_x, [[maybe_unused]] float h_y,
                                        [[maybe_unused]] float h_z) noexcept
{
  // TODO
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
DeviceDescriptor::OnEngineSensors(bool has_cht,
                                  Temperature cht,
                                  bool has_egt,
                                  Temperature egt,
                                  bool has_ignitions_per_second,
                                  float ignitions_per_second) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  if (has_ignitions_per_second) {
    basic.engine.ignitions_per_second = ignitions_per_second;
    basic.engine.ignitions_per_second_available.Update(basic.clock);

    if (config.engine_type != DeviceConfig::EngineType::NONE) {
      basic.engine.revolutions_per_second = ignitions_per_second *
        config.ignitions_to_revolutions_factors[static_cast<unsigned>(config.engine_type)];
      basic.engine.revolutions_per_second_available.Update(basic.clock);
    } else
      basic.engine.revolutions_per_second_available.Clear();
  } else {
    basic.engine.ignitions_per_second_available.Clear();
    basic.engine.revolutions_per_second_available.Clear();
  }

  if(has_cht){
    basic.engine.cht_temperature = cht;
    basic.engine.cht_temperature_available.Update(basic.clock);
  }else{
    basic.engine.cht_temperature_available.Clear();
  }
  if(has_egt){
    basic.engine.egt_temperature = egt;
    basic.engine.egt_temperature_available.Update(basic.clock);
  }else{
    basic.engine.egt_temperature_available.Clear();
  }
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
                                  [[maybe_unused]] int acc_x, [[maybe_unused]] int acc_y, int acc_z,
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
