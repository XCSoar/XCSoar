// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"
#include "thread/Mutex.hxx"
#include "Volatile.hpp"

#include <optional>

class NMEAInputLine;

class VegaDevice : public AbstractDevice {
private:
  Port &port;

  bool detected;

  Vega::VolatileData volatile_data;

  DeviceSettingsMap<int> settings;

public:
  VegaDevice(Port &_port)
    :port(_port),
     detected(false) {}

  /**
   * Write an integer setting to the Vega.
   */
  void SendSetting(const char *name, int value, OperationEnvironment &env);

  /**
   * Request an integer setting from the Vega.  The Vega will send the
   * value, but this method will not wait for that.
   */
  void RequestSetting(const char *name, OperationEnvironment &env);

  /**
   * Look up the given setting in the table of received values.  The
   * first element is a "found" flag, and if that is true, the second
   * element is the value.
   */
  [[gnu::pure]]
  std::optional<int> GetSetting(const char *name) const noexcept;

protected:
  bool PDVSC(NMEAInputLine &line, NMEAInfo &info);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure& pres,
              OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;
};
