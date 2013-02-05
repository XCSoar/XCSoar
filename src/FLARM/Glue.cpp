/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "FlarmDetails.hpp"
#include "FlarmNetReader.hpp"
#include "NameFile.hpp"
#include "Friends.hpp"
#include "Components.hpp"
#include "MergeThread.hpp"
#include "IO/DataFile.hpp"
#include "IO/TextWriter.hpp"
#include "Profile/FlarmProfile.hpp"
#include "LogFile.hpp"

/**
 * Loads the FLARMnet file
 */
static void
LoadFLARMnet(FlarmNetDatabase &db)
{
  NLineReader *reader = OpenDataTextFileA(_T("data.fln"));
  if (reader == NULL)
    return;

  unsigned num_records = FlarmNetReader::LoadFile(*reader, db);
  delete reader;

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

  TLineReader *reader = OpenDataTextFile(_T("xcsoar-flarm.txt"));
  if (reader != NULL) {
    LoadFlarmNameFile(*reader, db);
    delete reader;
  }
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
  Profile::Load(traffic_databases->flarm_colors);

  merge_thread->Resume();
}

void
SaveFlarmColors()
{
  if (traffic_databases != nullptr)
    Profile::Save(traffic_databases->flarm_colors);
}

/**
 * Saves XCSoars own FLARM details into the
 * corresponding file (xcsoar-flarm.txt)
   */
static void
SaveSecondary(FlarmNameDatabase &flarm_names)
{
  TextWriter *writer = CreateDataTextFile(_T("xcsoar-flarm.txt"));
  if (writer == NULL)
    return;

  SaveFlarmNameFile(*writer, flarm_names);
  delete writer;
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

