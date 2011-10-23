/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "OS/PathName.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s PORT BAUD\n", argv[0]);
    return EXIT_FAILURE;
  }

  PathName port_name(argv[1]);
  int baud = atoi(argv[2]);

#ifdef HAVE_POSIX
  TTYPort *port = new TTYPort(port_name, baud, *(Port::Handler *)NULL);
#else
  SerialPort *port = new SerialPort(port_name, baud, *(Port::Handler *)NULL);
#endif
  if (!port->Open()) {
    delete port;
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  port->SetRxTimeout(0x10000);

  char buffer[4096];
  while (true) {
    int nbytes = port->Read(buffer, sizeof(buffer));
    if (nbytes < 0)
      break;

    fwrite((const void *)buffer, 1, nbytes, stdout);
  }

  delete port;
  return EXIT_SUCCESS;
}
