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

#include "Kobo/WPASupplicant.hpp"
#include "OS/Args.hpp"
#include "Util/PrintException.cxx"

#include <array>

#include <errno.h>
#include <string.h>

int
main(int argc, char *argv[])
try {
  Args args(argc, argv, "PATH");
  const char *path = args.ExpectNext();
  args.ExpectEnd();

  WPASupplicant wpa_supplicant;
  if (!wpa_supplicant.Connect(path)) {
    fprintf(stderr, "Failed to connect to %s: %s\n", path, strerror(errno));
    return EXIT_FAILURE;
  }

  std::array<WifiConfiguredNetworkInfo, 64> networks;
  int n = wpa_supplicant.ListNetworks(&networks.front(), networks.size());
  if (n < 0) {
    fprintf(stderr, "LIST_NETWORKS failed\n");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < n; ++i) {
    const auto &network = networks[i];
    printf("%d\t%s\t%s\n", network.id,
           network.ssid.c_str(), network.bssid.c_str());
  }

  return EXIT_SUCCESS;
} catch (const std::exception &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
