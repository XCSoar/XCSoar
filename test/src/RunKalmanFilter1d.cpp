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

#include "OS/Args.hpp"
#include "IO/FileLineReader.hpp"
#include "Math/KalmanFilter1d.hpp"
#include "Util/PrintException.hxx"

#include <stdio.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileLineReaderA reader(path);

  KalmanFilter1d kalman_filter(0.0075);

  unsigned last_t = 0;
  double last_value;

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    const char *p = line;
    char *endptr;
    unsigned t = strtoul(p, &endptr, 10);
    if (endptr == line) {
      fprintf(stderr, "Malformed line: %s\n", line);
      return EXIT_FAILURE;
    }

    p = endptr;
    double value = strtod(p, &endptr);
    if (endptr == line) {
      fprintf(stderr, "Malformed line: %s\n", line);
      return EXIT_FAILURE;
    }

    if (last_t > 0 && t > last_t) {
      auto dt = (t - last_t) / 1000.;

      kalman_filter.Update(value, 0.05, dt);

      printf("%u %f %f %f %f\n", t,
             value, (value - last_value) / dt,
             kalman_filter.GetXAbs(), kalman_filter.GetXVel());
    }

    last_t = t;
    last_value = value;
  }

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
