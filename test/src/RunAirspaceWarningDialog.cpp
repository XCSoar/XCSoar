// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "ResourceLoader.hpp"
#include "io/FileReader.hxx"
#include "io/BufferedReader.hxx"
#include "io/ConfiguredFile.hpp"
#include "LocalPath.hpp"
#include "Components.hpp"

#include <memory>
#include <tchar.h>
#include <stdio.h>

void VisitDataFiles([[maybe_unused]] const TCHAR* filter,
                    [[maybe_unused]] File::Visitor &visitor) {}

InterfaceBlackboard CommonInterface::Private::blackboard;

ProtectedAirspaceWarningManager *airspace_warnings;

void
dlgAirspaceDetails([[maybe_unused]] ConstAirspacePtr the_airspace,
                   [[maybe_unused]] ProtectedAirspaceWarningManager *airspace_warnings)
{
}

void
ActionInterface::SetActiveFrequency([[maybe_unused]] const RadioFrequency freq,
                                    [[maybe_unused]] const TCHAR * freq_name,
                                    [[maybe_unused]] bool to_devices) noexcept
{
}

static void
LoadFiles(Airspaces &airspace_database)
{
  auto reader = OpenConfiguredFile(ProfileKeys::AirspaceFile);
  if (reader) {
    BufferedReader buffered_reader{*reader};
    ParseAirspaceFile(airspace_database, buffered_reader);
    airspace_database.Optimise();
  }
}

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  Airspaces airspace_database;

  AirspaceWarningConfig airspace_warning_config;
  airspace_warning_config.SetDefaults();

  AirspaceWarningManager airspace_warning(airspace_warning_config,
                                          airspace_database);
  airspace_warnings = new ProtectedAirspaceWarningManager(airspace_warning);

  LoadFiles(airspace_database);

  const auto range = airspace_database.QueryAll();
  auto it = range.begin();

  AirspaceInterceptSolution ais;
  for (unsigned i = 0; i < 5 && it != range.end(); ++i, ++it)
    airspace_warning.GetWarning(it->GetAirspacePtr())
      .UpdateSolution((AirspaceWarning::State)i, ais);

  dlgAirspaceWarningsShowModal(*airspace_warnings);

  delete airspace_warnings;
}
