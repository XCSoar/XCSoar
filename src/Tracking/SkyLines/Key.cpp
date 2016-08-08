/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Key.hpp"
#include "OS/UniqueFileDescriptor.hxx"

#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include "Util/ScopeExit.hxx"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

struct MacAddress {
  uint8_t data[6];
};

static bool
GetMacAddress(MacAddress &address, const char *interface_name)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
      return false;

    AtScopeExit(fd) { close(fd); };

    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface_name);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
      return false;

    memcpy(address.data, ifr.ifr_hwaddr.sa_data, 6);
    return true;
}

#endif

static constexpr uint64_t
MakeUint64_64(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
              uint64_t e, uint64_t f, uint64_t g, uint64_t h)
{
  return (a << 56) | (b << 48) | (c << 40) | (d << 32) |
    (e << 24) | (f << 16) | (g << 8) | h;
}

static constexpr uint64_t
MakeUint64(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
           uint8_t e, uint8_t f, uint8_t g, uint8_t h)
{
  return MakeUint64_64(a, b, c, d, e, f, g, h);
}

static constexpr uint64_t
ReplaceHighestByte64(uint64_t value, uint64_t new_highest_byte)
{
  return (value & ~(uint64_t(0xff) << 56)) | (new_highest_byte << 56);
}

uint64_t
SkyLinesTracking::GenerateKey()
{
#ifdef __linux__
  /* try to derive the key from the MAC address */
  MacAddress mac;
  if (GetMacAddress(mac, "wlan0") || GetMacAddress(mac, "eth0"))
    return MakeUint64(0x01, rand(), mac.data[0], mac.data[1],
                      mac.data[2], mac.data[3], mac.data[4], mac.data[5]);

  /* read the key from /dev/urandom */
  UniqueFileDescriptor fd;
  if (fd.OpenReadOnly("/dev/urandom")) {
    uint64_t key;
    if (fd.Read(&key, sizeof(key)) == sizeof(key))
      return ReplaceHighestByte64(key, 0x02);
  }
#endif

  /* if all else fails: use random(), which is bad but portable */
  return ReplaceHighestByte64((uint64_t(rand()) << 32) | rand(), 0x03);
}
