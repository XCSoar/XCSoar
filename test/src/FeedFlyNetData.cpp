/*
Copyright_License {

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

#include "Util/StaticString.hpp"
#include "Math/fixed.hpp"
#include "Time/PeriodClock.hpp"
#include "OS/SocketDescriptor.hpp"
#include "OS/SocketAddress.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  // Determine on which TCP port to connect to the server
  const char *tcp_port;
  if (argc < 2) {
    fprintf(stderr, "This program opens a TCP connection to a server which is assumed ");
    fprintf(stderr, "to be at 127.0.0.1, and sends artificial FlyNet vario data.\n\n");
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    fprintf(stderr, "Defaulting to port 4353\n");
    tcp_port = "4353";
  } else {
    tcp_port = argv[1];
  }

  // Convert IP address to binary form
  SocketAddress server_address;
  if (!server_address.Lookup("127.0.0.1", tcp_port, AF_INET)) {
    fprintf(stderr, "Failed to look up address\n");
    exit(EXIT_FAILURE);
  }

  // Create socket for the outgoing connection
  SocketDescriptor sock;
  if (!sock.CreateTCP()) {
    perror("Socket");
    exit(EXIT_FAILURE);
  }

  // Connect to the specified server
  if (!sock.Connect(server_address))
  {
    perror("Connect");
    exit(EXIT_FAILURE);
  }

  PeriodClock start_clock;
  start_clock.Update();

  PeriodClock pressure_clock;
  PeriodClock battery_clock;

  fixed pressure = fixed(101300);
  unsigned battery_level = 11;
  while (true) {
    if (pressure_clock.CheckUpdate(48)) {
      NarrowString<16> sentence;

      int elapsed_ms = start_clock.Elapsed();
      fixed elapsed = fixed(elapsed_ms) / 1000;
      fixed vario = sin(elapsed / 3) * cos(elapsed / 10) *
                    cos(elapsed / 20 + fixed(2)) * fixed(3);

      fixed pressure_vario = -vario * fixed(12.5);
      fixed delta_pressure = pressure_vario * 48 / 1000;
      pressure += delta_pressure;

      sentence = "_PRS ";
      sentence.AppendFormat("%08X", uround(pressure));
      sentence += "\n";

      sock.Write(sentence.c_str(), sentence.length());
    }

    if (battery_clock.CheckUpdate(11000)) {
      NarrowString<16> sentence;

      sentence = "_BAT ";
      if (battery_level <= 10)
        sentence.AppendFormat("%X", battery_level);
      else
        sentence += "*";

      sentence += "\n";
      sock.Write(sentence.c_str(), sentence.length());

      if (battery_level == 0)
        battery_level = 11;
      else
        battery_level--;
    }
  }

  return EXIT_SUCCESS;
}
