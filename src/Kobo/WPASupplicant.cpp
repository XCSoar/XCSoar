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

#include "WPASupplicant.hpp"
#include "OS/SocketAddress.hpp"
#include "OS/FileUtil.hpp"
#include "Util/NumberParser.hpp"
#include "Util/StaticString.hpp"

#include <string.h>
#include <stdlib.h>

bool
WPASupplicant::Connect(const char *path)
{
  Close();

  strcpy(local_path, "/tmp/xcsoar.XXXXXX");

  SocketAddress local_address;
  local_address.SetLocal(mktemp(local_path));

  SocketAddress peer_address;
  peer_address.SetLocal(path);

  return fd.Create(AF_LOCAL, SOCK_DGRAM, 0) &&
    fd.Bind(local_address) && fd.Connect(peer_address);
}

void
WPASupplicant::Close()
{
  if (!StringIsEmpty(local_path)) {
    File::Delete(local_path);
    local_path[0] = 0;
  }

  fd.Close();
}

bool
WPASupplicant::SendCommand(const char *cmd)
{
  const size_t length = strlen(cmd);
  return fd.Write(cmd, length) == ssize_t(length);
}

bool
WPASupplicant::ExpectResponse(const char *expected)
{
  const size_t length = strlen(expected);
  char buffer[4096];
  assert(length <= sizeof(buffer));

  return fd.Read(buffer, sizeof(buffer)) == ssize_t(length) &&
    memcmp(buffer, expected, length) == 0;
}

static bool
ParseStatusLine(WifiStatus &status, char *src)
{
  char *value = strchr(src, '=');
  if (value == nullptr)
    return false;

  *value++ = 0;

  if (StringIsEqual(src, "bssid"))
    status.bssid = value;
  else if (StringIsEqual(src, "ssid"))
    status.ssid = value;
  return true;
}

static bool
ParseStatus(WifiStatus &status, char *src)
{
  status.Clear();

  while (true) {
    char *eol = strchr(src, '\n');
    if (eol != nullptr)
      *eol = 0;

    if (!ParseStatusLine(status, src))
      break;

    if (eol == nullptr)
      break;

    src = eol + 1;
  }

  return true;
}

bool
WPASupplicant::Status(WifiStatus &status)
{
  if (!SendCommand("STATUS"))
    return false;

  char buffer[4096];
  ssize_t nbytes = fd.Read(buffer, sizeof(buffer) - 1);
  if (nbytes <= 0)
    return false;

  buffer[nbytes] = 0;

  return ParseStatus(status, buffer);
}

bool
WPASupplicant::Scan()
{
  return SendCommand("SCAN") && ExpectOK();
}

static bool
ParseScanResultsLine(WifiVisibleNetwork &dest, char *src)
{
  char *tab = strchr(src, '\t'); // seek "frequency"
  if (tab == nullptr)
    return false;

  *tab = 0;
  dest.bssid = src;

  src = tab + 1;

  src = strchr(src + 1, '\t'); // seek "signal level"
  if (src == nullptr)
    return false;

  ++src;

  char *endptr;
  dest.signal_level = ParseUnsigned(src, &endptr);
  if (endptr == src || *endptr != '\t')
    return false;

  src = endptr + 1;

  src = strchr(src, '\t'); // seek "ssid"
  if (src == nullptr)
    return false;

  ++src;
  tab = strchr(src, '\t');
  if (tab != nullptr)
    *tab = 0;

  if (StringIsEmpty(src))
    return false;

  dest.ssid = src;
  return true;
}

static int
ParseScanResults(WifiVisibleNetwork *dest, unsigned max, char *src)
{
  if (memcmp(src, "bssid", 5) != 0)
    return -1;

  src = strchr(src, '\n');
  if (src == nullptr)
    return -1;

  ++src;

  unsigned n = 0;
  do {
    char *eol = strchr(src, '\n');
    if (eol != nullptr)
      *eol = 0;

    if (!ParseScanResultsLine(dest[n], src))
      break;

    ++n;

    if (eol == nullptr)
      break;

    src = eol + 1;
  } while (n < max);

  return n;
}

int
WPASupplicant::ScanResults(WifiVisibleNetwork *dest, unsigned max)
{
  assert(dest != nullptr);
  assert(max > 0);

  if (!SendCommand("SCAN_RESULTS"))
    return -1;

  char buffer[4096];
  ssize_t nbytes = fd.Read(buffer, sizeof(buffer) - 1);
  if (nbytes <= 5)
    return -1;

  buffer[nbytes] = 0;

  return ParseScanResults(dest, max, buffer);
}

int
WPASupplicant::AddNetwork()
{
  if (!SendCommand("ADD_NETWORK"))
    return -1;

  char buffer[4096];
  ssize_t nbytes = fd.Read(buffer, sizeof(buffer));
  if (nbytes < 2 || buffer[nbytes - 1] != '\n')
    return -1;

  buffer[nbytes - 1] = 0;

  char *endptr;
  unsigned id = ParseUnsigned(buffer, &endptr);
  if (endptr == buffer || *endptr != 0)
    return -1;

  return id;
}

bool
WPASupplicant::SetNetworkString(unsigned id,
                                const char *name, const char *value)
{
  NarrowString<512> cmd;
  cmd.Format("SET_NETWORK %u %s \"%s\"", id, name, value);
  return SendCommand(cmd) && ExpectOK();
}

bool
WPASupplicant::SelectNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("SELECT_NETWORK %u", id);
  return SendCommand(cmd) && ExpectOK();
}

bool
WPASupplicant::EnableNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("ENABLE_NETWORK %u", id);
  return SendCommand(cmd) && ExpectOK();
}

bool
WPASupplicant::DisableNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("DISABLE_NETWORK %u", id);
  return SendCommand(cmd) && ExpectOK();
}

bool
WPASupplicant::RemoveNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("REMOVE_NETWORK %u", id);
  return SendCommand(cmd) && ExpectOK();
}

static bool
ParseListResultsLine(WifiConfiguredNetworkInfo &dest, char *src)
{
  char *endptr;
  dest.id = ParseUnsigned(src, &endptr);
  if (endptr == src || *endptr != '\t')
    return false;

  src = endptr + 1;

  char *tab = strchr(src, '\t');
  if (tab == nullptr)
    return false;

  *tab = 0;
  dest.ssid = src;

  src = tab + 1;

  tab = strchr(src, '\t');
  if (tab != nullptr)
    *tab = 0;

  dest.bssid = src;
  return true;
}

static int
ParseListResults(WifiConfiguredNetworkInfo *dest, unsigned max, char *src)
{
  if (memcmp(src, "network id", 10) != 0)
    return -1;

  src = strchr(src, '\n');
  if (src == nullptr)
    return -1;

  ++src;

  unsigned n = 0;
  do {
    char *eol = strchr(src, '\n');
    if (eol != nullptr)
      *eol = 0;

    if (!ParseListResultsLine(dest[n], src))
      break;

    ++n;

    if (eol == nullptr)
      break;

    src = eol + 1;
  } while (n < max);

  return n;
}

int
WPASupplicant::ListNetworks(WifiConfiguredNetworkInfo *dest, unsigned max)
{
  assert(dest != nullptr);
  assert(max > 0);

  if (!SendCommand("LIST_NETWORKS"))
    return -1;

  char buffer[4096];
  ssize_t nbytes = fd.Read(buffer, sizeof(buffer) - 1);
  if (nbytes <= 5)
    return -1;

  buffer[nbytes] = 0;

  return ParseListResults(dest, max, buffer);
}
