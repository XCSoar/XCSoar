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

#include "OS/Sleep.h"

#include "Device/Port/TCPPort.hpp"

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
  int tcp_port;
  if (argc < 2) {
    fprintf(stderr, "This program opens up a TCP server and listens for data. ");
    fprintf(stderr, "When data is received it is printed to stdout.\n\n");
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    fprintf(stderr, "Defaulting to port 4353\n");
    tcp_port = 4353;
  } else {
    tcp_port = atoi(argv[1]);
  }

  MyHandler handler;
  TCPPort port(handler);
  if (!port.Open(tcp_port) || !port.StartRxThread()) {
    fprintf(stderr, "Failed to open TCP port\n");
    return EXIT_FAILURE;
  }

  while (true)
    Sleep(10000);

  return EXIT_SUCCESS;
}
