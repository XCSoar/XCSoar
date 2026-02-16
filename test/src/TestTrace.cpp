// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "IGC/IGCExtensions.hpp"
#include "io/FileLineReader.hpp"
#include "system/ConvertPathName.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Engine/Trace/Vector.hpp"
#include "Printing.hpp"
#include "TestUtil.hpp"
#include "util/PrintException.hxx"

#include <windef.h>
#include <cassert>
#include <cstdio>

#include <tchar.h>

using namespace std::chrono;

static void
OnAdvance(Trace &trace, const GeoPoint &loc, const double alt,
          const TimeStamp t) noexcept
{
  if (t.IsDefined()) {
    const TracePoint point(loc, t.Cast<duration<unsigned>>(),
                           alt, 0, 0);
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
  Trace trace(seconds{1000}, Trace::null_time, ntrace);

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
              TimeStamp{fix.time.DurationSinceMidnight()});
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
    TestTrace(Path("test/data/09kc3ov3.igc"), n);
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
