// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Alternate.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Unordered/AlternateList.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Language/Language.hpp"
#include "Widget/CallbackWidget.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"

#include <stdio.h>
#include <tchar.h>


void
InfoBoxContentAlternateName::Update(InfoBoxData &data) noexcept
{
  if (!backend_components->protected_task_manager) {
    data.SetInvalid();
    return;
  }

  ProtectedTaskManager::Lease lease{*backend_components->protected_task_manager};
  const AlternateList &alternates = lease->GetAlternates();

  const AlternatePoint *alternate;
  if (!alternates.empty()) {
    if (index >= alternates.size())
      index = alternates.size() - 1;

    alternate = &alternates[index];
  } else {
    alternate = NULL;
  }

  data.FmtTitle(_("Altn {}"), index + 1);

  if (alternate == NULL || !CommonInterface::Basic().track_available) {
    data.SetInvalid();
    return;
  }

  data.SetComment(alternate->waypoint->name.c_str());

  // Set Value
  Angle Value = alternate->solution.vector.bearing -
    CommonInterface::Basic().track;

  data.SetValueFromBearingDifference(Value);

  // Set Color (blue/black)
  data.SetValueColor(alternate->solution.IsFinalGlide() ? 2 : 0);
}

bool
InfoBoxContentAlternateName::HandleClick() noexcept
{
  if (!data_components)
    return false;

  dlgAlternatesListShowModal(data_components->waypoints.get());
  return true;
}

void
InfoBoxContentAlternateGR::Update(InfoBoxData &data) noexcept
{
  if (!backend_components->protected_task_manager) {
    data.SetInvalid();
    return;
  }

  ProtectedTaskManager::Lease lease{*backend_components->protected_task_manager};
  const AlternateList &alternates = lease->GetAlternates();

  const AlternatePoint *alternate;
  if (!alternates.empty()) {
    if (index >= alternates.size())
      index = alternates.size() - 1;

    alternate = &alternates[index];
  } else {
    alternate = NULL;
  }

  data.FmtTitle(_T("Altn {} GR"), index + 1);

  if (alternate == NULL) {
    data.SetInvalid();
    return;
  }

  data.SetComment(alternate->waypoint->name.c_str());

  double gradient =
    ::AngleToGradient(alternate->solution.DestinationAngleGround());

  if (gradient < 0) {
    data.SetValueColor(0);
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    data.SetValueFromGlideRatio(gradient);
  } else {
    data.SetInvalid();
  }

  // Set Color (blue/black)
  data.SetValueColor(alternate->solution.IsFinalGlide() ? 2 : 0);
}

bool
InfoBoxContentAlternateGR::HandleClick() noexcept
{
  if (!data_components)
    return false;

  dlgAlternatesListShowModal(data_components->waypoints.get());
  return true;
}

void
InfoBoxContentAlternateAltDiff::Update(InfoBoxData &data) noexcept
{
  if (!backend_components->protected_task_manager) {
    data.SetInvalid();
    return;
  }

  ProtectedTaskManager::Lease lease{*backend_components->protected_task_manager};
  const AlternateList &alternates = lease->GetAlternates();

  const AlternatePoint *alternate;
  if (!alternates.empty()) {
    if (index >= alternates.size())
      index = alternates.size() - 1;

    alternate = &alternates[index];
  } else {
    alternate = NULL;
  }

  data.FmtTitle(_T("Altn {} AltD"), index + 1);

  if (alternate == NULL) {
    data.SetInvalid();
    return;
  }

  data.SetComment(alternate->waypoint->name.c_str());

  const auto &settings = CommonInterface::GetComputerSettings();
  auto altitude_difference =
    alternate->solution.SelectAltitudeDifference(settings.task.glide);
  data.SetValueFromArrival(altitude_difference);
}

bool
InfoBoxContentAlternateAltDiff::HandleClick() noexcept
{
  if (!data_components)
    return false;

  dlgAlternatesListShowModal(data_components->waypoints.get());
  return true;
}
