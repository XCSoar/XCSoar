// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Ptr.hpp"
#include "Airspace/AirspaceClass.hpp"

class Airspaces;
class ProtectedAirspaceWarningManager;
struct AirspaceLook;

void
dlgAirspaceDetails(ConstAirspacePtr airspace,
                   ProtectedAirspaceWarningManager *_airspace_warnings);

int dlgAirspacePatternsShowModal(const AirspaceLook &look);
void dlgAirspaceShowModal(bool colored);

void
ShowAirspaceListDialog(const Airspaces &airspace_database,
                       ProtectedAirspaceWarningManager *airspace_warnings);

bool ShowAirspaceClassRendererSettingsDialog(AirspaceClass selected = OTHER);
