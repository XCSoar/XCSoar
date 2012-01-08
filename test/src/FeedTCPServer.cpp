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

/*
 * This program opens a TCP server at the localhost at
 * port 4353 and feeds NMEA data read from stdin.
 */

#include "Device/Port/TCPPort.hpp"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class NullHandler : public Port::Handler {
public:
  virtual void DataReceived(const void *data, size_t length) {
  }
};

int
main(int argc, char **argv)
{
  // Determine on which TCP port to open the server
  int tcp_port;
  if (argc < 2) {
    fprintf(stderr, "This program opens up a TCP server and send NMEA data ");
    fprintf(stderr, "which is read from stdin.\n\n");
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    fprintf(stderr, "Defaulting to port 4353\n");
    tcp_port = 4353;
  } else {
    tcp_port = atoi(argv[1]);
  }

  NullHandler handler;
  TCPPort port(tcp_port, handler);
  if (!port.Open() || !port.StartRxThread()) {
    fprintf(stderr, "Failed to open TCP port\n");
    return EXIT_FAILURE;
  }

  char stamp[6] = "";

  char line[1024];
  long baudrate = 9600;
  useconds_t sleep_acc = 0;

  // Number of characters sent
  long c_count = 0;
  // Number of sentence packages sent
  long l_count = 0;

  while (fgets(line, sizeof(line), stdin) != NULL) {
    int l = strlen(line);
    c_count += l;
    long tsleep = l*1e6/(baudrate/10);
    usleep(tsleep);
    sleep_acc += tsleep;

    if (memcmp(line, "$GP", 3) == 0 &&
        (memcmp(line + 3, "GGA", 3) == 0 ||
         memcmp(line + 3, "RMC", 3) == 0) &&
        line[6] == ',' &&
        strncmp(stamp, line + 7, sizeof(stamp)) != 0) {
      /* the time stamp has changed - sleep for one second */
      usleep(1e6-sleep_acc);
      strncpy(stamp, line + 7, sizeof(stamp));
      sleep_acc = 0;
      l_count++;
      printf(".");
      fflush(stdout);
    }

    port.Write(line, l);
  }
  printf(">>>> Av %ld\n", c_count/l_count);
  return EXIT_SUCCESS;
}
