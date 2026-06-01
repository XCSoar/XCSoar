// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UtilsSettings.hpp"

bool DevicePortChanged = false;
bool AirspaceFileChanged = false;
bool WaypointFileChanged = false;
bool AirfieldFileChanged = false;
bool InputFileChanged = false;
bool MapFileChanged = false;
bool FlarmFileChanged = false;
bool RaspFileChanged = false;
bool ChecklistFileChanged = false;
bool UserRepositoriesListChanged = false;
bool LanguageChanged = false;
bool require_restart = false;

void
SettingsEnter() noexcept
{
}

void
SettingsLeave(const UISettings &)
{
}

void
SystemConfiguration()
{
}
