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

#ifndef IGC_REPLAY_GLUE_HPP
#define IGC_REPLAY_GLUE_HPP

#include "Replay/IgcReplay.hpp"
#include "PeriodClock.hpp"

class Logger;

class IgcReplayGlue: 
  public IgcReplay
{
  PeriodClock clock;

  Logger *logger;

public:
  IgcReplayGlue(Logger *_logger);

protected:
  virtual bool update_time();
  virtual void reset_time();
  virtual void on_reset();
  virtual void on_stop();
  virtual void on_bad_file();
  virtual void on_advance(const GeoPoint &loc,
                          const fixed speed, const Angle bearing,
                          const fixed alt, const fixed baroalt, const fixed t);
};

#endif
