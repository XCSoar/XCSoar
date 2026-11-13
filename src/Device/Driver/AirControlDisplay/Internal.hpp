// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "thread/Cond.hxx"
#include "thread/Mutex.hxx"
#include "time/PeriodClock.hpp"
#include "util/StaticString.hxx"

#include <atomic>
#include <string>
#include <string_view>

class NMEAInputLine;

class ACDDevice : public AbstractDevice {
  friend bool ParsePAAVS(NMEAInputLine &line, NMEAInfo &info,
                         ACDDevice *dev) noexcept;

  Port &port;
  PeriodClock status_clock;

  /**
   * Last COM standby channel (kHz) from the panel or from our own
   * PutStandbyFrequency.  CHN1 is read-only on the ACD; tuning active uses
   * CHN2 + swap + restore (see PutActiveFrequency).
   */
  std::atomic<unsigned> cached_com_standby_khz{0};
  std::atomic<bool> com_standby_khz_known{false};

  Mutex response_mutex;
  Cond response_cond;
  std::string_view pending_code, pending_item;
  StaticString<65> response_value;
  bool response_pending = false, response_ready = false;
  bool response_ok = false;

  bool ParseConfiguration(NMEAInputLine &line) noexcept;
  bool RequestConfiguration(const char *request, const char *code,
                            const char *item, std::string &value,
                            OperationEnvironment &env);

public:
  ACDDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const char *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const char *name,
                           OperationEnvironment &env) override;
  bool ExchangeRadioFrequencies(OperationEnvironment &env,
                                NMEAInfo &info) override;
  bool PutTransponderCode(TransponderCode code, OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic,
                          [[maybe_unused]] const DerivedInfo &calculated) override;

  /** Query an ACD configuration item while this device is borrowed. */
  bool ReadConfiguration(const char *code, const char *item,
                         std::string &value, OperationEnvironment &env);

  bool ReadConfigurationUnsigned(const char *code, const char *item,
                                 unsigned &value, unsigned max,
                                 OperationEnvironment &env);

  /** Set an item and wait for the device's acknowledgement. */
  bool WriteConfiguration(const char *code, const char *item,
                          unsigned value, OperationEnvironment &env);
};
