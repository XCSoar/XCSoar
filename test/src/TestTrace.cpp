/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Engine/Trace/Trace.hpp"
#include "Engine/Task/TaskStats/CommonStats.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Printing.hpp"
#include "TestUtil.hpp"

#include <windef.h>
#include <assert.h>
#include <cstdio>

class IgcReplayGlue:
  public IgcReplay
{
public:
  Trace trace;
  bool error;

  IgcReplayGlue(unsigned ntrace): 
    trace(Trace::null_time, ntrace),
    error(false) {}

  void SetFilename(const char *name);

protected:
  virtual bool update_time(const fixed mintime);
  virtual void reset_time();
  virtual void on_reset() {}
  virtual void on_stop() {}
  virtual void on_bad_file() { error = true; }
  virtual void on_advance(const GeoPoint &loc, const fixed speed,
                          const Angle bearing, const fixed alt,
                          const fixed baroalt, const fixed t);

};

bool
IgcReplayGlue::update_time(const fixed mintime)
{
  t_simulation += fixed_one;
  t_simulation = std::max(mintime, t_simulation);
  return true;
}

void
IgcReplayGlue::reset_time()
{
  t_simulation = fixed_zero;
}

void
IgcReplayGlue::on_advance(const GeoPoint &loc, const fixed speed,
                          const Angle bearing, const fixed alt,
                          const fixed baroalt, const fixed t)
{
  AIRCRAFT_STATE new_state;
  new_state.Location = loc;
  new_state.Speed = speed;
  new_state.NavAltitude = alt;
  new_state.TrackBearing = bearing;
  new_state.Time = t;
  new_state.AltitudeAGL = alt;

  trace.append(new_state);

  trace.optimise_if_old();
}

void
IgcReplayGlue::SetFilename(const char *name)
{
#ifdef _UNICODE
  TCHAR tname[MAX_PATH];
  int length = ::MultiByteToWideChar(CP_ACP, 0, name, -1, tname, MAX_PATH);
  if (length == 0)
    return;

  IgcReplay::SetFilename(tname);
#else
  IgcReplay::SetFilename(name);
#endif
}

static bool
TestTrace(const char *filename, unsigned ntrace)
{
  IgcReplayGlue replay(ntrace);
  replay.SetFilename(filename);
  replay.Start();
  assert(!replay.error);

  putchar('#');
  for (int i = 1; replay.Update(); i++) {
    if (i % 500 == 0) {
      putchar('.');
      fflush(stdout);
    }
  }
  putchar('\n');
  return true;
}


int main(int argc, char **argv)
{
  assert(argc >= 2);

  plan_tests(12);

  for (unsigned i=2; i<14; i++) {
    unsigned n = pow(2,i);
    char buf[100];
    sprintf(buf," trace size %d", n);
    ok(TestTrace(argv[1], n),buf, 0);
  }
  return 0;
}
