// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueFlightLogger.hpp"
#include "Blackboard/LiveBlackboard.hpp"

GlueFlightLogger::GlueFlightLogger(LiveBlackboard &_blackboard)
  :blackboard(_blackboard)
{
  blackboard.AddListener(*this);
}

GlueFlightLogger::~GlueFlightLogger()
{
  blackboard.RemoveListener(*this);
}

void
GlueFlightLogger::OnCalculatedUpdate(const MoreData &basic,
                                     const DerivedInfo &calculated)
{
  FlightLogger::Tick(basic, calculated);
}
