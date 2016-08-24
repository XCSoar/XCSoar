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
#include "IO/DataFile.hpp"
#include "IO/LineReader.hpp"
#include "IO/TextWriter.hpp"
#include "Profile/FlarmProfile.hpp"
#include "Profile/Current.hpp"
#include "LogFile.hpp"
#include "Util/Error.hxx"

/**
 * Loads the FLARMnet file
 */
static void
LoadFLARMnet(FlarmNetDatabase &db)
{
  auto reader = OpenDataTextFileA(_T("data.fln"), IgnoreError());
  if (!reader)
    return;

  unsigned num_records = FlarmNetReader::LoadFile(*reader, db);
  if (num_records > 0)
    LogFormat("%u FLARMnet ids found", num_records);
}

/**
 * Opens XCSoars own FLARM details file, parses it and
 * adds its entries as FlarmLookupItems
 * @see AddSecondaryItem
 */
static void
LoadSecondary(FlarmNameDatabase &db)
{
  LogFormat("OpenFLARMDetails");

  auto reader = OpenDataTextFile(_T("xcsoar-flarm.txt"), IgnoreError());
  if (reader)
    LoadFlarmNameFile(*reader, db);
}

void
LoadFlarmDatabases()
{
  if (traffic_databases != nullptr)
    return;

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
{
  auto writer = CreateDataTextFile(_T("xcsoar-flarm.txt"));
  if (!writer)
    return;

  SaveFlarmNameFile(*writer, flarm_names);
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

