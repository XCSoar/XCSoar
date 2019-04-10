/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Glue.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"
#include "FlarmNetReader.hpp"
#include "NameFile.hpp"
#include "Components.hpp"
#include "MergeThread.hpp"
#include "LocalPath.hpp"
#include "IO/DataFile.hpp"
#include "IO/LineReader.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "Profile/FlarmProfile.hpp"
#include "Profile/Current.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"

/**
 * Loads the FLARMnet file
 */
static void
LoadFLARMnet(FlarmNetDatabase &db)
try {
  auto path = Profile::GetPath(ProfileKeys::FlarmFile);
  if (path.IsNull()) {
    return;
  }

  unsigned num_records = FlarmNetReader::LoadFile(path, db);
  if (num_records > 0)
    LogFormat("%u FLARMnet ids found", num_records);
} catch (...) {
  LogError(std::current_exception());
}

/**
 * Opens XCSoars own FLARM details file, parses it and
 * adds its entries as FlarmLookupItems
 * @see AddSecondaryItem
 */
static void
LoadSecondary(FlarmNameDatabase &db)
try {
  LogFormat("OpenFLARMDetails");

  auto reader = OpenDataTextFile(_T("xcsoar-flarm.txt"));
  LoadFlarmNameFile(*reader, db);
} catch (...) {
  LogError(std::current_exception());
}

void
LoadFlarmDatabases()
{
  if (traffic_databases != nullptr)
    return;

  ReloadFlarmDatabases();
}

void
ReloadFlarmDatabases()
{
  traffic_databases = new TrafficDatabases();

  /* the MergeThread must be suspended, because it reads the FLARM
     databases */
  merge_thread->Suspend();

  LoadSecondary(traffic_databases->flarm_names);
  LoadFLARMnet(traffic_databases->flarm_net);
  Profile::Load(Profile::map, traffic_databases->flarm_colors);

  merge_thread->Resume();
}

void
SaveFlarmColors()
{
  if (traffic_databases != nullptr)
    Profile::Save(Profile::map, traffic_databases->flarm_colors);
}

/**
 * Saves XCSoars own FLARM details into the
 * corresponding file (xcsoar-flarm.txt)
   */
static void
SaveSecondary(FlarmNameDatabase &flarm_names)
try {
  FileOutputStream fos(LocalPath(_T("xcsoar-flarm.txt")));
  BufferedOutputStream bos(fos);
  SaveFlarmNameFile(bos, flarm_names);
  bos.Flush();
  fos.Commit();
} catch (...) {
  LogError(std::current_exception());
}

void
SaveFlarmNames()
{
  if (traffic_databases != nullptr)
    SaveSecondary(traffic_databases->flarm_names);
}

void
DeinitTrafficGlobals()
{
  delete traffic_databases;
  traffic_databases = nullptr;
}
