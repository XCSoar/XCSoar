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

#ifndef XCSOAR_FLYTEC_DEVICE_HPP
#define XCSOAR_FLYTEC_DEVICE_HPP

#include "tchar.h"
#include "Device/Driver.hpp"

class Port;
struct NMEAInfo;
class OperationEnvironment;
class RecordedFlightList;
struct RecordedFlightInfo;
class NMEAInputLine;

class FlytecDevice : public AbstractDevice
{
  Port &port;
  double last_time;

public:
  FlytecDevice(Port &_port):port(_port), last_time(0) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, NMEAInfo &info) override;

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;

  bool DownloadFlight(const RecordedFlightInfo &flight, Path path,
                      OperationEnvironment &env) override;

private:
  bool ParseFLYSEN(NMEAInputLine &line, NMEAInfo &info);
};

#endif
