/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef NMEA_REPLAY_GLUE_HPP
#define NMEA_REPLAY_GLUE_HPP

#include "Replay/NmeaReplay.hpp"
#include "PeriodClock.hpp"
#include "Device/NullPort.hpp"

class Device;
class NMEAParser;

class NmeaReplayGlue:
  public NmeaReplay
{
  NullPort port;
  NMEAParser *parser;
  Device *device;

  PeriodClock clock;

public:
  NmeaReplayGlue();
  virtual ~NmeaReplayGlue();

  virtual void Start();
  virtual void Stop();

protected:
  virtual bool update_time();
  virtual void reset_time();
  virtual void on_bad_file();
  virtual void on_sentence(const char *line);
};

#endif
