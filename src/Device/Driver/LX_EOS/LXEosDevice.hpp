// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Declaration.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/LX_Eos.hpp"
#include "Device/Port/Port.hpp"
#include "Device/SettingsMap.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "NMEA/DeviceInfo.hpp"
#include "NMEA/InputLine.hpp"
#include "thread/Mutex.hxx"

/**
 * @brief Struct to hold last known settings of the device
 *
 */
struct VarioSettings
{
  float mc = 0;          // Mc Cready in m/s
  float bugs = 0;        // Bugs in percent (lower value = less bugs)
  float bal = 1;         // Glider mass divided by polar reference mass
  bool uptodate = false; // Setting were received from device at least once
  PolarCoefficients device_polar; // Polar coefficients from device
  float device_reference_mass;    // Reference mass of device's polar
  bool device_reference_mass_uptodate = false;
  GlidePolar xcsoar_polar; // Polar coefficients used by XCSoar
};

/**
 * @brief Struct to hold last known altitude offset
 *
 */
struct AltitudeOffset
{
  bool known = false; // alt_offset in known (LXWP3 was received at least once)

  /* Device is considered not having fw bug where alt_offset doesn't get
   * updated in flight Is false unless proven true by device type from LXWP1 */
  bool reliable = false;

  int16_t last_received = 0; // alt_offset from last LXWP3, in feet
  float meters = 0;          // most recent alt_offset converted to meters
};

class LXEosDevice : public AbstractDevice
{
  Port& port;

public:
  explicit LXEosDevice(Port& _port)
    : port(_port)
  {
  }

private:
  VarioSettings vario_settings; // last known settings of the device
  Mutex settings_mutex;

  AltitudeOffset altitude_offset; // last known settings of the device

  bool LXWP0(NMEAInputLine& line, NMEAInfo& info);
  bool LXWP2(NMEAInputLine& line, NMEAInfo& info);
  bool LXWP3(NMEAInputLine& line, NMEAInfo& info);

  /**
   * @brief Decides if the device has reliable alt_offset parameter in the
   * LXWP3 sentence. Eos is all right (tested on fw 1.7), but Era is reported
   * to have a bug where the alt_offset doesn't get updated if QNH is changed
   * in-flight
   *
   * @param device
   * @return true
   * @return false
   */
  static bool HasReliableAltOffset(DeviceInfo device);

  /**
   * @brief Calculates ratio od dry mass to polar reference mass. This is used
   * to convert water ballast overload. The reference mass of the Eos device
   * may be different from the one in XCSoar. Polar coefficients are used to
   * estimate the reference mass of device's polar.
   *
   * @param settings
   */
  static void CalculateDevicePolarReferenceMass(VarioSettings& settings);

  /**
   * @brief Compare coefficients of polars
   *
   * @param polar1
   * @param polar2
   * @return true: coefficients are the same
   * @return false: coefficients are different or at least one polar is invalid
   */
  static bool ComparePolarCoefficients(PolarCoefficients polar1,
                                       PolarCoefficients polar2);

  /**
   * @brief Sends settings from vario_settings struct to device
   *
   * @param env
   * @return true if successful
   * @return false if vario_settings are not uptodate (previous settings are
   * unknown)
   */
  bool SendNewSettings(OperationEnvironment& env);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool EnableNMEA(OperationEnvironment& env) override;
  bool ParseNMEA(const char* line, struct NMEAInfo& info) override;
  bool PutBugs(double bugs, OperationEnvironment& env) override;
  bool PutMacCready(double mc, OperationEnvironment& env) override;
  bool PutBallast(double fraction,
                  double overload,
                  OperationEnvironment& env) override;
  void OnCalculatedUpdate(const MoreData& basic,
                          const DerivedInfo& calculated) override;
};
