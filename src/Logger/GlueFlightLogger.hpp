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

#ifndef XCSOAR_GLUE_FLIGHT_LOGGER_HPP
#define XCSOAR_GLUE_FLIGHT_LOGGER_HPP

#include "Blackboard/BlackboardListener.hpp"
#include "FlightLogger.hpp"

class LiveBlackboard;

/**
 * This class glues FlightLogger and LiveBlackboard together.
 */
class GlueFlightLogger : public FlightLogger, private NullBlackboardListener {
  LiveBlackboard &blackboard;

  double last_time;
  bool last_on_ground, last_flying;

  /**
   * Set to the most recently observed start time.  It gets cleared
   * after a landing has been logged.
   */
  BrokenDateTime start_time;

public:
  GlueFlightLogger(LiveBlackboard &blackboard);

  /* this destructor is virtual even though it does not need to be;
     clang emits a bogus warning, because this class has virtual
     methods inherited from BlackboardListener */
  virtual ~GlueFlightLogger();

  void Reset();

private:
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);
};

#endif
