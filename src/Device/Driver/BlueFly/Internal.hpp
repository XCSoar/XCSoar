// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "Math/KalmanFilter1d.hpp"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"

#include <cassert>
#include <string_view>

struct NMEAInfo;

class BlueFlyDevice : public AbstractDevice {
public:
  struct BlueFlySettings {
    unsigned version;

    double volume;
    static const char VOLUME_NAME[];
    static constexpr unsigned VOLUME_MAX = 1000;
    static constexpr unsigned VOLUME_MULTIPLIER = 1000;

    unsigned output_mode;
    static const char OUTPUT_MODE_NAME[];
    static constexpr unsigned OUTPUT_MODE_MAX = 3;

    [[gnu::const]]
    static unsigned ExportVolume(double value) {
      assert(value >= 0);
      unsigned v = unsigned(value * VOLUME_MULTIPLIER);

      assert(v <= VOLUME_MAX);
      return v;
    }

    unsigned ExportVolume() const {
      return ExportVolume(volume);
    }

    [[gnu::const]]
    static unsigned ExportOutputMode(unsigned value) {
      assert(value <= OUTPUT_MODE_MAX);
      return value;
    }

    unsigned ExportOutputMode() const {
      return ExportOutputMode(output_mode);
    }

    void Parse(std::string_view name, unsigned long value);
};

private:
  Port &port;
  Mutex mutex_settings;
  Cond settings_cond;
  bool settings_ready;
  BlueFlySettings settings;
  char *settings_keys;

  KalmanFilter1d kalman_filter;

  bool ParseBAT(const char *content, NMEAInfo &info);
  bool ParsePRS(const char *content, NMEAInfo &info);
  bool ParseBFV(const char *content, NMEAInfo &info);
  bool ParseBST(const char *content, NMEAInfo &info);
  bool ParseSET(const char *content, NMEAInfo &info);

  void WriteDeviceSetting(const char *name, int value,
                          OperationEnvironment &env);

public:
  explicit BlueFlyDevice(Port &_port);
  ~BlueFlyDevice();

  /**
   * Request the current settings configuration from the BlueFly Vario.
   * The BlueFly Vario will send the values, but this method will not
   * wait for that.
   */
  void RequestSettings(OperationEnvironment &env);

  /**
   * Wait for the BlueFly Vario to send its settings.
   * @timeout the timeout in milliseconds.
   *
   * @return true if the settings were received, false if a timeout occured.
   */
  bool WaitForSettings(unsigned int timeout);

  /**
   * Copy the available settings to the caller.
   */
  [[gnu::pure]]
  BlueFlySettings GetSettings() noexcept;

  /**
   * Write settings to the BlueFly Vario.
   *
   * The BlueFly Vario does not indicate whether it has understood and
   * processed it.
   */
  void WriteDeviceSettings(const BlueFlySettings &settings,
                           OperationEnvironment &env);

  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};
