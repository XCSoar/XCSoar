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

#ifndef NMEA_REPLAY_HPP
#define NMEA_REPLAY_HPP

#include "AbstractReplay.hpp"
#include "Time/ReplayClock.hpp"
#include "Device/Port/NullPort.hpp"

#include <memory>

class NLineReader;
class NMEAParser;
class Device;
struct DeviceConfig;
struct NMEAInfo;

class NmeaReplay: public AbstractReplay
{
  std::unique_ptr<NLineReader> reader;

  NMEAParser *parser;
  NullPort port;
  Device *device;

  ReplayClock clock;

public:
  NmeaReplay(std::unique_ptr<NLineReader> &&_reader,
             const DeviceConfig &config);
  ~NmeaReplay();

  bool Update(NMEAInfo &data) override;

protected:
  bool ParseLine(const char *line, NMEAInfo &data);

private:
  bool ReadUntilRMC(NMEAInfo &data);
};

#endif
