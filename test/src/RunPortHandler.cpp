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
#include "OS/Sleep.h"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>

class MyHandler : public Port::Handler {
public:
  virtual void DataReceived(const void *data, size_t length) {
    fwrite(data, 1, length, stdout);
  }
};

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s PORT BAUD\n", argv[0]);
    return EXIT_FAILURE;
  }

  PathName port_name(argv[1]);
  int baud = atoi(argv[2]);

  MyHandler handler;
#ifdef HAVE_POSIX
  TTYPort *port = new TTYPort(port_name, baud, handler);
#else
  SerialPort *port = new SerialPort(port_name, baud, handler);
#endif
  if (!port->Open()) {
    delete port;
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  while (true)
    Sleep(10000);

  return EXIT_SUCCESS;
}
