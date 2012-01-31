/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Device/Driver/Vega/Internal.hpp"
#include "OS/PathName.hpp"
#include "Profile/DeviceConfig.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  if (argc < 4) {
    fprintf(stderr, "Usage: %s PORT BAUD [NAME=VALUE] [NAME] ...\n", argv[0]);
    return EXIT_FAILURE;
  }

  PathName port_name(argv[1]);

  DeviceConfig config;
  config.Clear();
  config.baud_rate = atoi(argv[2]);

#ifdef HAVE_POSIX
  TTYPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#else
  SerialPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#endif
  if (!port.Open()) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  VegaDevice device(port);

  for (int i = 3; i < argc; ++i) {
    const char *p = argv[i];
    char *q = strdup(p);
    char *v = strchr(q, '=');
    if (v == NULL) {
      if (!device.RequestSetting(q))
        printf("Error\n");
    } else {
      *v++ = 0;
      if (!device.SendSetting(q, atoi(v)))
        printf("Error\n");
    }

    free(q);
  }

  return EXIT_SUCCESS;
}
