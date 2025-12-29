// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"
#include "FlarmNetReader.hpp"
#include "NameFile.hpp"
#include "MessagingFile.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "MergeThread.hpp"
#include "LocalPath.hpp"
#include "io/DataFile.hpp"
#include "io/Reader.hxx"
#include "io/BufferedReader.hxx"
#include "io/LineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "Profile/FlarmProfile.hpp"
#include "Profile/Current.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "time/PeriodClock.hpp"

/**
 * Loads the FLARMnet file
 */
static void
LoadFLARMnet(FlarmNetDatabase &db) noexcept
try {
  auto path = Profile::GetPath(ProfileKeys::FlarmFile);
  if (path == nullptr) {
    return;
  }

  unsigned num_records = FlarmNetReader::LoadFile(path, db);
  if (num_records > 0)
    LogFormat("FLARMnet IDs found: %u", num_records);
} catch (...) {
  LogError(std::current_exception());
}

/**
 * Opens XCSoars own FLARM details file, parses it and
 * adds its entries as FlarmLookupItems
 * @see AddSecondaryItem
 */
static void
LoadSecondary(FlarmNameDatabase &db) noexcept
try {
  LogString("OpenFLARMDetails");

  auto reader = OpenDataFile(_T("xcsoar-flarm.txt"));
  BufferedReader buffered_reader{*reader};
  LoadFlarmNameFile(buffered_reader, db);
} catch (...) {
  LogError(std::current_exception());
}

static void
LoadFlarmMessagingData(FlarmMessagingDatabase &db) noexcept
try {
  LogString("OpenFLARMMessages");

  auto reader = OpenDataFile(_T("flarm-msg-data.csv"));
  BufferedReader buffered_reader{*reader};
  unsigned num_records = LoadFlarmMessagingFile(buffered_reader, db);
  if (num_records > 0)
    LogFormat("%u FLARM messaging records found", num_records);
  db.MarkSaved();  // Initialize hash baseline after load
} catch (...) {
  LogError(std::current_exception());
}

void
LoadFlarmDatabases() noexcept
{
  if (traffic_databases != nullptr)
    return;

  ReloadFlarmDatabases();
}

void
ReloadFlarmDatabases() noexcept
{
  /* the MergeThread must be suspended, because it reads the FLARM
     databases */
  backend_components->merge_thread->Suspend();

  delete traffic_databases;
  traffic_databases = nullptr;

  traffic_databases = new TrafficDatabases();

  LoadSecondary(traffic_databases->flarm_names);
  LoadFlarmMessagingData(traffic_databases->flarm_messages);
  LoadFLARMnet(traffic_databases->flarm_net);
  Profile::Load(Profile::map, traffic_databases->flarm_colors);

  backend_components->merge_thread->Resume();
}

void
SaveFlarmColors() noexcept
{
  if (traffic_databases != nullptr)
    Profile::Save(Profile::map, traffic_databases->flarm_colors);
}

/**
 * Saves XCSoars own FLARM details into the
 * corresponding file (xcsoar-flarm.txt)
   */
static void
SaveSecondary(FlarmNameDatabase &flarm_names) noexcept
try {
  FileOutputStream fos(LocalPath(_T("xcsoar-flarm.txt")));
  BufferedOutputStream bos(fos);
  SaveFlarmNameFile(bos, flarm_names);
  bos.Flush();
  fos.Commit();
} catch (...) {
  LogError(std::current_exception());
}

static void
SaveMessaging(FlarmMessagingDatabase &flarm_messages) noexcept
try {
  FileOutputStream fos(LocalPath(_T("flarm-msg-data.csv")));
  BufferedOutputStream bos(fos);
  SaveFlarmMessagingFile(bos, flarm_messages);
  bos.Flush();
  fos.Commit();
} catch (...) {
  LogError(std::current_exception());
}

void
SaveFlarmNames() noexcept
{
  if (traffic_databases != nullptr)
    SaveSecondary(traffic_databases->flarm_names);
}

void
SaveFlarmMessaging() noexcept
{
  if (traffic_databases != nullptr)
    SaveMessaging(traffic_databases->flarm_messages);
}

void
SaveFlarmMessagingPeriodic() noexcept
{
  using namespace std::chrono;

  static PeriodClock save_clock;
  static constexpr auto interval = minutes(5);

  if (traffic_databases == nullptr)
    return;

  if (!traffic_databases->flarm_messages.HasUpdates())
    return;

  if (!save_clock.Check(interval))
    return;

  SaveMessaging(traffic_databases->flarm_messages);
  traffic_databases->flarm_messages.MarkSaved();
  save_clock.Update();
}

void
DeinitTrafficGlobals() noexcept
{
  delete traffic_databases;
  traffic_databases = nullptr;
}
