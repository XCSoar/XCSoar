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
#include "OS/FileUtil.hpp"
#include "Util/StaticString.hxx"
#include "Util/PrintException.hxx"
#include "Compiler.h"

#include <cstdio>

#include <stdlib.h>

class FlightCheck {
  StaticString<64> name;

  unsigned year, month, day;

  IGCFix previous, slow, fast, takeoff, landing;
  bool previous_valid, takeoff_valid, landing_valid;
  unsigned slow_count, fast_count;

public:
  FlightCheck(const TCHAR *_name)
    :name(_name),
     year(0), month(0), day(0),
     previous_valid(false), takeoff_valid(false),
     slow_count(0), fast_count(0) {}

  void date(unsigned _year, unsigned _month, unsigned _day) {
    year = _year;
    month = _month;
    day = _day;
  }

  void print_flight() {
    _tprintf(_T("%s,%04u-%02u-%02u,%02u:%02u,%02u:%02u\n"), name.c_str(),
             year, month, day,
             takeoff.time.hour, takeoff.time.minute,
             landing.time.hour, landing.time.minute);
  }

  void fix(const IGCFix &fix);
  void finish();
};

void
FlightCheck::fix(const IGCFix &fix)
{
  if (!fix.gps_valid)
    return;

  if (previous_valid && fix.time > previous.time) {
    auto distance = fix.location.Distance(previous.location);
    auto speed = distance / (fix.time.GetSecondOfDay() - previous.time.GetSecondOfDay());
    if (speed > 15) {
      if (fast_count == 0)
        fast = fix;

      ++fast_count;
    } else
      fast_count = 0;

    if (speed < 5) {
      if (slow_count == 0)
        slow = fix;
      ++slow_count;
    } else
      slow_count = 0;

    if (takeoff_valid) {
      if (slow_count > 10) {
        landing = slow;
        landing_valid = true;

        print_flight();
        takeoff_valid = landing_valid = false;
      }
    } else {
      if (fast_count > 10) {
        takeoff = fast;
        takeoff_valid = true;
      }
    }
  }

  previous = fix;
  previous_valid = true;
}

void
FlightCheck::finish()
{
  if (takeoff_valid) {
    landing = previous;
    landing_valid = true;

    print_flight();
  }
}

class IGCFileVisitor : public File::Visitor {
  void Visit(Path path, Path filename) override;
};

void
IGCFileVisitor::Visit(Path path, Path filename)
{
  FileLineReaderA reader(path);

  IGCExtensions extensions;
  extensions.clear();

  FlightCheck flight(filename.c_str());
  char *line;
  while ((line = reader.ReadLine()) != NULL) {
    unsigned day, month, year;

    IGCFix fix;
    if (IGCParseFix(line, extensions, fix))
      flight.fix(fix);
    else if (sscanf(line, "HFDTE%02u%02u%02u", &day, &month, &year)) {
      /* damn you, Y2K bug! */
      if (year > 80)
        year += 1900;
      else
        year += 2000;

      flight.date(year, month, day);
    }
  }

  flight.finish();
}

int main(gcc_unused int argc, gcc_unused char **argv)
try {
  IGCFileVisitor visitor;
  Directory::VisitSpecificFiles(Path(_T(".")), _T("*.igc"), visitor);
  return 0;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
