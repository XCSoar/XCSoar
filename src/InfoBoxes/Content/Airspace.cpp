// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Airspace.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Airspace/NearestAirspace.hpp"

void
UpdateInfoBoxNearestAirspaceHorizontal(InfoBoxData &data) noexcept
{
  NearestAirspace nearest = NearestAirspace::FindHorizontal(CommonInterface::Basic(),
                                                            backend_components->GetAirspaceWarnings(),
                                                            *data_components->airspaces);
  if (!nearest.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromDistance(nearest.distance);
  data.SetComment(nearest.airspace->GetName());
}

void
UpdateInfoBoxNearestAirspaceVertical(InfoBoxData &data) noexcept
{
  NearestAirspace nearest = NearestAirspace::FindVertical(CommonInterface::Basic(),
                                                          CommonInterface::Calculated(),
                                                          backend_components->GetAirspaceWarnings(),
                                                          *data_components->airspaces);
  if (!nearest.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromArrival(nearest.distance);
  data.SetComment(nearest.airspace->GetName());
}
