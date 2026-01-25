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
#include "Dialogs/Airspace/Airspace.hpp"
#include "Engine/Airspace/Airspaces.hpp"

void
InfoBoxNearestAirspaceHorizontal::Update(InfoBoxData &data) noexcept
{
  if (backend_components == nullptr || data_components == nullptr ||
      data_components->airspaces == nullptr) {
    data.SetInvalid();
    return;
  }

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

bool
InfoBoxNearestAirspaceHorizontal::HandleClick() noexcept
{
  if (backend_components == nullptr || data_components == nullptr ||
      data_components->airspaces == nullptr)
    return false;

  NearestAirspace nearest = NearestAirspace::FindHorizontal(CommonInterface::Basic(),
                                                            backend_components->GetAirspaceWarnings(),
                                                            *data_components->airspaces);

  if (!nearest.IsDefined())
    return false;

  for (const auto &i : data_components->airspaces->QueryWithinRange(CommonInterface::Basic().location,
                                                                      nearest.distance * 1.1)){
    if (&i.GetAirspace() == nearest.airspace){
      ConstAirspacePtr airspace = i.GetAirspacePtr();
      dlgAirspaceDetails(airspace, backend_components->GetAirspaceWarnings());
      return true;
    }
  }

  return false;
}

void
InfoBoxNearestAirspaceVertical::Update(InfoBoxData &data) noexcept
{
  if (backend_components == nullptr || data_components == nullptr ||
      data_components->airspaces == nullptr) {
    data.SetInvalid();
    return;
  }

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

bool
InfoBoxNearestAirspaceVertical::HandleClick() noexcept
{
  if (backend_components == nullptr || data_components == nullptr ||
      data_components->airspaces == nullptr)
    return false;

  NearestAirspace nearest = NearestAirspace::FindVertical(CommonInterface::Basic(),
                                                          CommonInterface::Calculated(),
                                                          backend_components->GetAirspaceWarnings(),
                                                          *data_components->airspaces);

  if (!nearest.IsDefined())
    return false;

  for (const auto &i : data_components->airspaces->QueryInside(CommonInterface::Basic().location)){
    if (&i.GetAirspace() == nearest.airspace){
      ConstAirspacePtr airspace = i.GetAirspacePtr();
      dlgAirspaceDetails(airspace, backend_components->GetAirspaceWarnings());
      return true;
    }
  }

  return false;
}
