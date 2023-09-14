// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Kobo/WPASupplicant.hpp"
#include "system/Args.hpp"
#include "util/PrintException.cxx"

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
  wpa_supplicant.Connect(path);

  std::array<WifiConfiguredNetworkInfo, 64> networks;
  const std::size_t n = wpa_supplicant.ListNetworks(&networks.front(), networks.size());

  for (std::size_t i = 0; i < n; ++i) {
    const auto &network = networks[i];
    printf("%d\t%s\t%s\n", network.id,
           network.ssid.c_str(), network.bssid.c_str());
  }

  return EXIT_SUCCESS;
} catch (const std::exception &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
