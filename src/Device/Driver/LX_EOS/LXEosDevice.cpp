// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LXEosDevice.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/DeviceInfo.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/MoreData.hpp"
#include "Units/Units.hpp"
#include "util/ByteOrder.hxx"
#include "util/VersionNumber.hxx"

using std::string_view_literals::operator""sv;

void
LXEosDevice::LinkTimeout()
{
  const std::lock_guard lock{ settings_mutex };
  vario_settings.uptodate = false;
  altitude_offset.known = false;
  altitude_offset.reliable = false;
  vario_settings.device_polar.SetInvalid();
  vario_settings.device_reference_mass_uptodate = false;
}

bool
LXEosDevice::EnableNMEA(OperationEnvironment& env)
{
  /*
   * Set up the NMEA sentences sent by the vario:
   *
   * - LXWP0 every second (most important data)
   * - LXWP1 every 7 seconds (device info)
   * - LXWP2 every 5 seconds (MC, Bugs, Ballast settings)
   * - LXWP3 every 3 seconds (used for alt_offset (QNH) )
   *
   * It seems that LXWP3 does not get sent if it is to be sent at the same
   * time as LXWP2, so choosing odd numbers to reduce overlaps
   *
   * LXWP2 is also sent automatically whenever vario settings are changed by
   * pilot. Periodical sending is there only as a backup sync method.
   */

  // Set intervals of periodic messages
  PortWriteNMEA(port, "PFLX0,LXWP0,1,LXWP1,7,LXWP2,5,LXWP3,3", env);
  port.Flush();

  return true;
}

bool
LXEosDevice::PutMacCready(double mc, OperationEnvironment& env)
{
  const std::lock_guard lock{ settings_mutex };
  vario_settings.mc = mc;
  return SendNewSettings(env);
}

bool
LXEosDevice::PutBallast([[maybe_unused]] double fraction,
                        double overload,
                        OperationEnvironment& env)
{
  const std::lock_guard lock{ settings_mutex };
  if (vario_settings.device_reference_mass_uptodate) {
  // TODO: If XCSoar's definition of overload factor gets changed, change this:
    float total_mass = overload * vario_settings.xcsoar_polar.GetDryMass();
  // to this:
    // float total_mass = overload * vario_settings.xcsoar_polar.GetReferenceMass();
    vario_settings.bal = total_mass / vario_settings.device_reference_mass;
    return SendNewSettings(env);
  }
  return false;
}

void
LXEosDevice::OnCalculatedUpdate([[maybe_unused]] const MoreData& basic,
                                const DerivedInfo& calculated)
{
  const std::lock_guard lock{ settings_mutex };
  auto& old_polar = vario_settings.xcsoar_polar;
  auto& new_polar = calculated.glide_polar_safety;
  if (!ComparePolarCoefficients(old_polar.GetCoefficients(),
                                new_polar.GetCoefficients()) ||
      old_polar.GetDryMass() != new_polar.GetDryMass() ||
      old_polar.GetReferenceMass() != new_polar.GetReferenceMass()) {
    vario_settings.xcsoar_polar = calculated.glide_polar_safety;
    CalculateDevicePolarReferenceMass(vario_settings);
  }
}

bool
LXEosDevice::PutBugs(double bugs, OperationEnvironment& env)
{
  const std::lock_guard lock{ settings_mutex };
  vario_settings.bugs = (1.0 - bugs) * 100.0;
  return SendNewSettings(env);
}

bool
LXEosDevice::ParseNMEA(const char* String, NMEAInfo& info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$LXWP0"sv)
    return LXWP0(line, info);
  else if (type == "$LXWP1"sv) {
    // LXWP1 sentence is identical to LXNAV, using method from LXNAV driver
    LXDevice::LXWP1(line, info.device);
    altitude_offset.reliable = HasReliableAltOffset(info.device);
    return true;
  } else if (type == "$LXWP2"sv)
    return LXWP2(line, info);
  else if (type == "$LXWP3"sv)
    return LXWP3(line, info);
  else
    return false;
}

bool
LXEosDevice::SendNewSettings(OperationEnvironment& env)
{
  if (vario_settings.uptodate == true) {
    char buffer[64];
    snprintf(buffer,
             64,
             "PFLX2,%.1f,%.2f,%.0f,,,,,",
             vario_settings.mc,
             vario_settings.bal,
             vario_settings.bugs);

    PortWriteNMEA(port, buffer, env);
    return true;
  } else
    return false;
}

void
LXEosDevice::WriteAndWaitForACK(const std::span<const std::byte> &message,
                                OperationEnvironment &env)
{
  port.FullFlush(env, std::chrono::milliseconds(50),
                 std::chrono::milliseconds(200));
  port.FullWrite(message, env, communication_timeout);
  port.WaitForByte(ACK, env, communication_timeout);
}

void
LXEosDevice::CopyStringSpacePadded(char dest[], const TCHAR src[],
                                   const size_t len)
{
  bool src_end_reached = false;
  for (uint8_t i = 0; i < (len - 1); i++) {
    if (!src_end_reached)
      if (src[i] == 0) src_end_reached = true;
    dest[i] = src_end_reached ? '\x20' : src[i];
  }

  dest[len - 1] = 0;
}

PackedBE32
LXEosDevice::ConvertCoord(Angle coord)
{
  int32_t value = static_cast<int32_t>(coord.Degrees() * 60000.0);
  return *reinterpret_cast<uint32_t *>(&value);
}

bool
LXEosDevice::HasReliableAltOffset(DeviceInfo device)
{
  /*
   * The altitude offset is considered to be reliable (gets updated if QNH
   * is changed in-flight) on LX Eos device with firmware version >= 1.7.
   * LX Era is known to have the issue on recent version.
   * It is unknown whether older versions on Eos have the issue
   *
   * If the altitude offset is considered reliable, the true altitude provided
   * by device is converted into standard altitude and then passed to XCSoar.
   * Otherwise, the altitude is passed as-is using the ProvideBaroAltitudeTrue.
   * Correctness of XCSoar's standard altitude will then depend on correctness
   * of QNH setting in XCSoar. The altitude offset is used to set XCsoar's QNH
   * in both cases.
   *
   * The "Era bug" manifests as an inability to synchronize QNH change in
   * flight. Altitudes will still be correct if the QNH is changed manually in
   * XCSoar.
   */

  if (device.product == "LX Eos"sv) {
    if(VersionNumber(device.software_version) >= VersionNumber(1,7))
      return true;
  }

  // LX Era and older LX Eos
  return false;
}

void
LXEosDevice::CalculateDevicePolarReferenceMass(VarioSettings& settings)
{
  if (!settings.device_polar.IsValid() || !settings.xcsoar_polar.IsValid()) {
    // One or both polars are unknown, cannot get reference mass
    settings.device_reference_mass_uptodate = false;
    return;
  }

  // LX polar coefficients are such that v == 1 corresponds to 100 kph
  float polar_v_scale = Units::ToSysUnit(100, Unit::KILOMETER_PER_HOUR);
  float dev_a = settings.device_polar.a / polar_v_scale / polar_v_scale;
  float dev_c = settings.device_polar.c;

  auto xcs_polar = settings.xcsoar_polar.GetCoefficients();

  /*
   * Calculate and compare best LD velocities to get reference mass of device's
   * polar v2/v1 = sqrt(m2/m1) -> m2 = m1 * (v2/v1)^2
   */
  float bestLD_v_xcs = sqrt(dev_c / dev_a);
  float bestLD_v_dev = sqrt(xcs_polar.c / xcs_polar.a);
  float v_ratio = (bestLD_v_dev / bestLD_v_xcs);
  settings.device_reference_mass =
    settings.xcsoar_polar.GetReferenceMass() * v_ratio * v_ratio;

  /*
   * The calculation won't give precise result. If the calculated reference
   * mass is higher than the actual value, it causes trouble: When the ballast
   * in XCSoar is set to zero, the corresponding ballast in vario would have
   * negative mass. Such value gets ignored by the vario (it keeps previous
   * value) rather than clipping it to zero. To ensure that this problem won't
   * arise, the calculated reference mass is slightly decreased so that the
   * inaccuracy will cause the ballast weight on the vario to be slightly
   * higher than in XCSoar.
   */

  // Decrease by 5% and floor to nearest multiple of 10
  settings.device_reference_mass *= 0.95f;
  settings.device_reference_mass = 10.0f * floor(settings.device_reference_mass / 10.0f);

  settings.device_reference_mass_uptodate = true;
}

bool
LXEosDevice::ComparePolarCoefficients(PolarCoefficients polar1,
                                      PolarCoefficients polar2)
{
  if (!polar1.IsValid())
    return false;

  if (!polar2.IsValid())
    return false;

  if (fabs(polar1.a - polar2.a) >= 0.01)
    return false;
  if (fabs(polar1.b - polar2.b) >= 0.01)
    return false;
  if (fabs(polar1.c - polar2.c) >= 0.01)
    return false;
  return true;
}
