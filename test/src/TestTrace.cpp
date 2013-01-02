/* Copyright_License {

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

#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "IO/FileLineReader.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Engine/Trace/Vector.hpp"
#include "Printing.hpp"
#include "TestUtil.hpp"

#include <windef.h>
#include <assert.h>
#include <cstdio>

static void
OnAdvance(Trace &trace, const GeoPoint &loc, const fixed alt, const fixed t)
{
  if (t>fixed(1)) {
    const TracePoint point(loc, unsigned(t), alt, fixed(0), 0);
    trace.push_back(point);
  }
// get the trace, just so it's included in timing
  TracePointVector v;
  trace.GetPoints(v);
  if (trace.size()>1) {
//    assert(abs(v.size()-trace.size())<2);
  }
}

static bool
TestTrace(const char *filename, unsigned ntrace, bool output=false)
{
  FileLineReaderA reader(filename);
  if (reader.error()) {
    fprintf(stderr, "Failed to open %s\n", filename);
    return false;
  }

  printf("# %d", ntrace);  
  Trace trace(1000, ntrace);

  char *line;
  int i = 0;
  for (; (line = reader.ReadLine()) != NULL; i++) {
    if (output && (i % 500 == 0)) {
      putchar('.');
      fflush(stdout);
    }

    IGCFix fix;
    if (!IGCParseFix(line, fix) || !fix.gps_valid)
      continue;

    OnAdvance(trace,
               fix.location,
               fixed(fix.gps_altitude),
               fixed(fix.time.GetSecondOfDay()));
  }
  putchar('\n');
  printf("# samples %d\n", i);
  return true;
}


int main(int argc, char **argv)
{
  if (argc < 3) {
    unsigned n = 100;
    if (argc > 1) {
      n = atoi(argv[1]);
    }
    TestTrace("test/data/09kc3ov3.igc", n);
  } else {
    assert(argc >= 3);
    unsigned n = atoi(argv[2]);
    plan_tests(n);
    
    for (unsigned i=2; i<2+n; i++) {
      unsigned nt = pow(2,i);
      char buf[100];
      sprintf(buf," trace size %d", nt);
      ok(TestTrace(argv[1], nt),buf, 0);
    }
  }
  return 0;
}
