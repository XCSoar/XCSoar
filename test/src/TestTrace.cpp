/* Copyright_License {

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

#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "IGC/IGCExtensions.hpp"
#include "IO/FileLineReader.hpp"
#include "OS/ConvertPathName.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Engine/Trace/Vector.hpp"
#include "Printing.hpp"
#include "TestUtil.hpp"
#include "Util/PrintException.hxx"

#include <windef.h>
#include <assert.h>
#include <cstdio>

static void
OnAdvance(Trace &trace, const GeoPoint &loc, const double alt, const double t)
{
  if (t>1) {
    const TracePoint point(loc, unsigned(t), alt, 0, 0);
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
TestTrace(Path filename, unsigned ntrace, bool output=false)
{
  FileLineReaderA reader(filename);

  printf("# %d", ntrace);  
  Trace trace(1000, ntrace);

  IGCExtensions extensions;
  extensions.clear();

  char *line;
  int i = 0;
  for (; (line = reader.ReadLine()) != NULL; i++) {
    if (output && (i % 500 == 0)) {
      putchar('.');
      fflush(stdout);
    }

    IGCFix fix;
    if (!IGCParseFix(line, extensions, fix) || !fix.gps_valid)
      continue;

    OnAdvance(trace,
               fix.location,
               fix.gps_altitude,
               fix.time.GetSecondOfDay());
  }
  putchar('\n');
  printf("# samples %d\n", i);
  return true;
}


int main(int argc, char **argv)
try {
  if (argc < 3) {
    unsigned n = 100;
    if (argc > 1) {
      n = atoi(argv[1]);
    }
    TestTrace(Path(_T("test/data/09kc3ov3.igc")), n);
  } else {
    assert(argc >= 3);
    unsigned n = atoi(argv[2]);
    plan_tests(n);
    
    for (unsigned i=2; i<2+n; i++) {
      unsigned nt = pow(2,i);
      char buf[100];
      sprintf(buf," trace size %d", nt);
      ok(TestTrace(PathName(argv[1]), nt),buf, 0);
    }
  }
  return 0;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
