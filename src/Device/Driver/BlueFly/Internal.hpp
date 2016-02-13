/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_BLUEFLYVARIO_INTERNAL_HPP
#define XCSOAR_BLUEFLYVARIO_INTERNAL_HPP

#include "Device/Driver.hpp"
#include "Math/KalmanFilter1d.hpp"
#include "NMEA/Info.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hxx"

#include <assert.h>

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

    gcc_const
    static unsigned ExportVolume(double value) {
      assert(value >= 0);
      unsigned v = unsigned(value * VOLUME_MULTIPLIER);

      assert(v <= VOLUME_MAX);
      return v;
    }

    unsigned ExportVolume() const {
      return ExportVolume(volume);
    }

    gcc_const
    static unsigned ExportOutputMode(unsigned value) {
      assert(value <= OUTPUT_MODE_MAX);
      return value;
    }

    unsigned ExportOutputMode() const {
      return ExportOutputMode(output_mode);
    }

    void Parse(const char *name, unsigned long value);
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

  bool WriteDeviceSetting(const char *name, int value, OperationEnvironment &env);

public:
  explicit BlueFlyDevice(Port &_port);
  ~BlueFlyDevice();

  /**
   * Request the current settings configuration from the BlueFly Vario.
   * The BlueFly Vario will send the values, but this method will not
   * wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the BlueFly Vario has understood and processed it)
   */
  bool RequestSettings(OperationEnvironment &env);

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
  void GetSettings(BlueFlySettings &settings_r);

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

#endif
