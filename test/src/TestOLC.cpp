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
#include "Engine/Task/Tasks/OnlineContest.hpp"
#include "Engine/Navigation/Aircraft.hpp"

#include <windef.h>
#include <assert.h>
#include <cstdio>

CommonStats stats_classic;
Trace full_trace_classic;
Trace sprint_trace_classic(9000, 2, 300);
OnlineContest olc_classic(OLC_Classic, stats_classic,
                          full_trace_classic, full_trace_classic);

CommonStats stats_fai;
Trace full_trace_fai;
Trace sprint_trace_fai(9000, 2, 300);
OnlineContest olc_fai(OLC_FAI, stats_fai,
                      full_trace_fai, sprint_trace_fai);

CommonStats stats_sprint;
Trace full_trace_sprint;
Trace sprint_trace_sprint(9000, 2, 300);
OnlineContest olc_sprint(OLC_Sprint, stats_sprint,
                         full_trace_sprint, sprint_trace_sprint);

class IgcReplayGlue:
  public IgcReplay
{
public:
  bool error;

  IgcReplayGlue() : error(false) {}

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

  full_trace_classic.append(new_state);
  sprint_trace_classic.append(new_state);
  olc_classic.update_sample(new_state);

  full_trace_classic.optimise_if_old();
  sprint_trace_classic.optimise_if_old();


  full_trace_fai.append(new_state);
  sprint_trace_fai.append(new_state);
  olc_fai.update_sample(new_state);

  full_trace_fai.optimise_if_old();
  sprint_trace_fai.optimise_if_old();


  full_trace_sprint.append(new_state);
  sprint_trace_sprint.append(new_state);
  olc_sprint.update_sample(new_state);

  full_trace_sprint.optimise_if_old();
  sprint_trace_sprint.optimise_if_old();
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

static void
TestOLC(const char *filename)
{
  IgcReplayGlue replay;
  replay.SetFilename(filename);
  replay.Start();
  assert(!replay.error);

  for (int i = 1; replay.Update(); i++) {
    if (i % 500 == 0) {
      putchar('.');
    }
  }

  while (!olc_classic.update_idle()) {}
  while (!olc_fai.update_idle()) {}
  while (!olc_sprint.update_idle()) {}

  putchar('\n');

  printf("classic: %.2f km / %.3f min / %.2f km/h\n",
          (double)stats_classic.distance_olc / 1000,
          (double)stats_classic.time_olc / 60,
          (double)stats_classic.speed_olc * 3.6);
  printf("fai: %.2f km / %.3f min / %.2f km/h\n",
          (double)stats_fai.distance_olc / 1000,
          (double)stats_fai.time_olc / 60,
          (double)stats_fai.speed_olc * 3.6);
  printf("sprint: %.2f km / %.3f min / %.2f km/h\n",
          (double)stats_sprint.distance_olc / 1000,
          (double)stats_sprint.time_olc / 60,
          (double)stats_sprint.speed_olc * 3.6);
}

int main(int argc, char **argv)
{
  assert(argc >= 2);

  TestOLC(argv[1]);

  return 0;
}
