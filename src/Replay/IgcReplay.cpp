/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Replay/IgcReplay.hpp"
#include "Replay/IGCParser.hpp"

#include <algorithm>

#include "Navigation/GeoPoint.hpp"
#include "StringUtil.hpp"

IgcReplay::IgcReplay() :
  AbstractReplay(),
  cli(fixed(0.98)),
  reader(NULL)
{
  FileName[0] = _T('\0');
}

bool
IgcReplay::ScanBuffer(const TCHAR* buffer, IGCFix &fix)
{
  return IGCParseFix(buffer, fix);
}

bool
IgcReplay::ReadPoint(IGCFix &fix)
{
  TCHAR *buffer;

  while ((buffer = reader->read()) != NULL) {
    if (ScanBuffer(buffer, fix))
      return true;
  }

  return false;
}

bool
IgcReplay::update_time()
{
  const fixed t_simulation_last = t_simulation;

  t_simulation += fixed_one * TimeScale;

  return (t_simulation > t_simulation_last);
}

void
IgcReplay::reset_time()
{
  t_simulation = fixed_zero;
}

void
IgcReplay::Stop()
{
  CloseFile();

  on_stop();

  Enabled = false;
}

void
IgcReplay::Start()
{
  if (Enabled)
    Stop();

  if (!OpenFile()) {
    on_bad_file();
    return;
  }

  cli.Reset();
  reset_time();
  on_reset();

  Enabled = true;
}

const TCHAR*
IgcReplay::GetFilename()
{
  return FileName;
}

void
IgcReplay::SetFilename(const TCHAR *name)
{
  if (!name || string_is_empty(name))
    return;

  if (_tcscmp(FileName, name) != 0)
    _tcscpy(FileName, name);
}

bool
IgcReplay::Update()
{
  if (!Enabled)
    return false;

  if (positive(t_simulation) && !update_time())
    return true;

  // if need a new point
  while (cli.NeedData(t_simulation)) {
    IGCFix fix;
    if (!ReadPoint(fix)) {
      Stop();
      return false;
    }

    if (positive(fix.time))
      cli.Update(fix.time, fix.location,
                 fix.gps_altitude, fix.pressure_altitude);
  }

  if (!positive(t_simulation))
    t_simulation = cli.GetMaxTime();

  const CatmullRomInterpolator::Record r = cli.Interpolate(t_simulation);
  const GeoVector v = cli.GetVector(t_simulation);
  on_advance(r.loc, v.Distance, v.Bearing, r.alt, r.palt, t_simulation);

  return true;
}

bool
IgcReplay::OpenFile()
{
  if (reader)
    return true;

  if (string_is_empty(FileName))
    return false;

  reader = new FileLineReader(FileName);
  if (!reader->error())
    return true;

  return false;
}

void
IgcReplay::CloseFile()
{
  if (!reader)
    return;

  delete reader;
  reader = NULL;
}
