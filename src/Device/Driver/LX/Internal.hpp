/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DRIVER_LX_INTERNAL_HPP
#define XCSOAR_DEVICE_DRIVER_LX_INTERNAL_HPP

#include "Protocol.hpp"
#include "Device/Driver.hpp"
#include "Thread/Mutex.hpp"

#include <stdint.h>

class LXDevice: public AbstractDevice
{
  enum class Mode : uint8_t {
    UNKNOWN,
    NMEA,
    PASS_THROUGH,
    COMMAND,
  };

  Port &port;

  unsigned bulk_baud_rate;

  Mutex mutex;
  Mode mode;
  bool busy;
  unsigned old_baud_rate;

public:
  LXDevice(Port &_port, unsigned _bulk_baud_rate)
    :port(_port), bulk_baud_rate(_bulk_baud_rate),
     mode(Mode::UNKNOWN), old_baud_rate(0) {}

protected:
  bool EnableCommandMode(OperationEnvironment &env);

public:
  virtual void LinkTimeout();
  virtual bool EnableNMEA(OperationEnvironment &env);

  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);

  virtual bool PutBallast(fixed fraction, fixed overload,
                          OperationEnvironment &env);
  virtual bool PutBugs(fixed bugs, OperationEnvironment &env);
  virtual bool PutMacCready(fixed mc, OperationEnvironment &env);
  virtual bool PutQNH(const AtmosphericPressure &pres,
                      OperationEnvironment &env);

  virtual bool EnablePassThrough(OperationEnvironment &env);

  virtual bool Declare(const Declaration &declaration, const Waypoint *home,
                       OperationEnvironment &env);

  virtual void OnSysTicker(const DerivedInfo &calculated);

  virtual bool ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env);
  virtual bool DownloadFlight(const RecordedFlightInfo &flight,
                              const TCHAR *path,
                              OperationEnvironment &env);
};

#endif
