// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "Math/KalmanFilter1d.hpp"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"

#include <cassert>
#include <cmath>
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

    bool audio_when_connected;
    static const char AUDIO_WHEN_CONNECTED_NAME[];

    double lift_threshold;
    static const char LIFT_THRESHOLD_NAME[];

    double lift_off_threshold;
    static const char LIFT_OFF_THRESHOLD_NAME[];

    double sink_threshold;
    static const char SINK_THRESHOLD_NAME[];

    double sink_off_threshold;
    static const char SINK_OFF_THRESHOLD_NAME[];

    static constexpr unsigned THRESHOLD_MULTIPLIER = 100;
    static constexpr double THRESHOLD_MAX = 10;

    unsigned output_mode;
    static const char OUTPUT_MODE_NAME[];
    static constexpr unsigned OUTPUT_MODE_MAX = 6;

    unsigned output_frequency;
    static const char OUTPUT_FREQUENCY_NAME[];
    static constexpr unsigned OUTPUT_FREQUENCY_MIN = 1;
    static constexpr unsigned OUTPUT_FREQUENCY_MAX = 50;

    [[gnu::const]]
    static unsigned ExportVolume(double value) {
      if (value < 0)
        value = 0;
      unsigned v = unsigned(std::lround(value * VOLUME_MULTIPLIER));
      if (v > VOLUME_MAX)
        v = VOLUME_MAX;
      return v;
    }

    unsigned ExportVolume() const {
      return ExportVolume(volume);
    }

    [[gnu::const]]
    static unsigned ExportBoolean(bool value) {
      return value ? 1 : 0;
    }

    unsigned ExportAudioWhenConnected() const {
      return ExportBoolean(audio_when_connected);
    }

    [[gnu::const]]
    static unsigned ExportThreshold(double value) {
      if (value < 0)
        value = 0;
      if (value > THRESHOLD_MAX)
        value = THRESHOLD_MAX;
      return unsigned(std::lround(value * THRESHOLD_MULTIPLIER));
    }

    unsigned ExportLiftThreshold() const {
      return ExportThreshold(lift_threshold);
    }

    unsigned ExportLiftOffThreshold() const {
      return ExportThreshold(lift_off_threshold);
    }

    unsigned ExportSinkThreshold() const {
      return ExportThreshold(sink_threshold);
    }

    unsigned ExportSinkOffThreshold() const {
      return ExportThreshold(sink_off_threshold);
    }

    [[gnu::const]]
    static unsigned ExportOutputMode(unsigned value) {
      assert(value <= OUTPUT_MODE_MAX);
      return value;
    }

    unsigned ExportOutputMode() const {
      return ExportOutputMode(output_mode);
    }

    [[gnu::const]]
    static unsigned ExportOutputFrequency(unsigned value) {
      if (value < OUTPUT_FREQUENCY_MIN)
        return OUTPUT_FREQUENCY_MIN;
      if (value > OUTPUT_FREQUENCY_MAX)
        return OUTPUT_FREQUENCY_MAX;
      return value;
    }

    unsigned ExportOutputFrequency() const {
      return ExportOutputFrequency(output_frequency);
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
  bool ParseTMP(const char *content, NMEAInfo &info);
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
  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight, Path path,
                      OperationEnvironment &env) override;
};
