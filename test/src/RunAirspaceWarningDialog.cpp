// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "ActionInterface.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Airspace/Patterns.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Components.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "Main.hpp"
#include "Operation/Operation.hpp"
#include "Profile/Profile.hpp"
#include "ResourceLoader.hpp"
#include "UIGlobals.hpp"
#include "io/BufferedReader.hxx"
#include "io/ConfiguredFile.hpp"
#include "io/FileReader.hxx"

#include <memory>
#include <stdio.h>
#include <tchar.h>

void VisitDataFiles([[maybe_unused]] const char* filter,
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
                                    [[maybe_unused]] const char * freq_name,
                                    [[maybe_unused]] bool to_devices) noexcept
{
}

static void
LoadFiles(Airspaces &airspace_database)
{
  NullOperationEnvironment test_operation_environment;
  const auto paths = Profile::GetMultiplePaths(ProfileKeys::AirspaceFileList,
                                               AIRSPACE_FILE_PATTERNS);
  for (auto it = paths.begin(); it < paths.end(); it++) {
    ParseAirspaceFile(airspace_database, *it, test_operation_environment);
  }
  airspace_database.Optimise();
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
