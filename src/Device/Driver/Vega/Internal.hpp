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

#ifndef XCSOAR_VEGA_INTERNAL_HPP
#define XCSOAR_VEGA_INTERNAL_HPP

#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"
#include "Thread/Mutex.hpp"
#include "Volatile.hpp"

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
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Vega has understood and processed it)
   */
  bool SendSetting(const char *name, int value, OperationEnvironment &env);

  /**
   * Request an integer setting from the Vega.  The Vega will send the
   * value, but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Vega has understood and processed it)
   */
  bool RequestSetting(const char *name, OperationEnvironment &env);

  /**
   * Look up the given setting in the table of received values.  The
   * first element is a "found" flag, and if that is true, the second
   * element is the value.
   */
  gcc_pure
  std::pair<bool, int> GetSetting(const char *name) const;

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

#endif
