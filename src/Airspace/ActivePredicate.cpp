// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ActivePredicate.hpp"
#include "ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"

bool
ActiveAirspacePredicate::operator()(const AbstractAirspace &airspace) const
{
  if (warnings != nullptr) {
    ProtectedAirspaceWarningManager::Lease lease(*warnings);
    return lease->IsActive(airspace);
  } else
    /* fallback */
    return airspace.IsActive();
}
