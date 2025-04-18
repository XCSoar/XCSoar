// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
