/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "net/AllocatedSocketAddress.hxx"
#include "system/Error.hxx"
#include "util/NumberParser.hpp"
#include "util/StaticString.hxx"
#include "util/StringCompare.hxx"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

void
WPASupplicant::Connect(const char *path)
{
  Close();

  AllocatedSocketAddress peer_address;
  peer_address.SetLocal(path);

  if (!fd.Create(AF_LOCAL, SOCK_DGRAM, 0))
    throw MakeErrno("Failed to create socket");

  if (!fd.AutoBind())
    throw MakeErrno("Failed to bind socket");

  if (!fd.Connect(peer_address))
    throw FormatErrno("Failed to connect to %s", path);
}

void
WPASupplicant::Close() noexcept
{
  if (fd.IsDefined())
    fd.Close();
}

void
WPASupplicant::SendCommand(const char *cmd)
{
  /* discard any previous responses that may be left in the socket's
     receive queue, maybe because the last command failed */
  ReadDiscard();

  const size_t length = strlen(cmd);
  const ssize_t nbytes = fd.Write(cmd, length);
  if (nbytes < 0)
    throw MakeErrno("Failed to send command to wpa_supplicant");

  if (std::size_t(nbytes) != length)
    throw std::runtime_error("Short send to wpa_supplicant");
}

void
WPASupplicant::ExpectResponse(const char *expected)
{
  const size_t length = strlen(expected);
  char buffer[4096];
  assert(length <= sizeof(buffer));

  std::size_t nbytes = ReadTimeout(buffer, sizeof(buffer));
  if (nbytes != length ||
      memcmp(buffer, expected, length) != 0)
    throw std::runtime_error{"Unexpected wpa_supplicant response"};
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
  SendCommand("STATUS");

  char buffer[4096];
  const std::size_t nbytes = ReadTimeout(buffer, sizeof(buffer) - 1);
  if (nbytes == 0)
    throw std::runtime_error{"wpa_supplicant closed the socket"};

  buffer[nbytes] = 0;

  return ParseStatus(status, buffer);
}

/*
 * Scan Results look like:
 * bssid                   frequency     signal_level  flags                                                   ssid
 * bc:14:01:e1:d6:78       2412          178           [WPA-PSK-TKIP+CCMP][WPA2-PSK-TKIP+CCMP][WPS][ESS]       FunnyMaple
 * 00:22:a4:b8:f3:31       2437          185           [WEP][ESS]                                              BELL778
 * 98:fc:11:3e:58:ea       2462          169           [WPA-PSK-TKIP+CCMP][WPA2-PSK-TKIP+CCMP][WPS][ESS]       Cisco54414
 * bc:14:01:e1:d6:79       2412          176           [WPA2-PSK-CCMP][ESS]
 * 44:94:fc:36:22:48       2412          173           [WPA2-PSK-CCMP][WPS][ESS]                               NETGEAR14
 *
 * Fields are delimited by single tabs.
 *
 * Items of interest which are:
 * - bssid  binary ssid
 * - signal_level a number, bigger is better.
 * - network type. A wireless router may support one or all of WEP, WPA, and WPA2.
 *                 WPA and WPA2 are handled the same. WPA/WPA2 are preferred over WEP.
 * - ssid ascii ssid. ssid could be empty if ssid broadcast is disabled.
 */
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

  tab = strchr(src, '\t'); // seek "ssid"
  if (tab == nullptr)
    return false;

  *tab = 0;

  // src points to the flags.
  if (strstr(src, "WPA") != NULL)
    dest.security = WPA_SECURITY;
  else if (strstr(src, "WEP") != NULL)
    dest.security = WEP_SECURITY;
  else
    dest.security = OPEN_SECURITY;

  src = tab + 1;

  tab = strchr(src, '\t');
  if (tab != nullptr)
    *tab = 0;

  if (StringIsEmpty(src))
    return false;

  dest.ssid = src;
  return true;
}

static std::size_t
ParseScanResults(WifiVisibleNetwork *dest, std::size_t max, char *src)
{
  if (memcmp(src, "bssid", 5) != 0)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  src = strchr(src, '\n');
  if (src == nullptr)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  ++src;

  std::size_t n = 0;
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

std::size_t
WPASupplicant::ScanResults(WifiVisibleNetwork *dest, unsigned max)
{
  assert(dest != nullptr);
  assert(max > 0);

  SendCommand("SCAN_RESULTS");

  char buffer[4096];
  ssize_t nbytes = ReadTimeout(buffer, sizeof(buffer) - 1);
  if (nbytes <= 5)
    throw std::runtime_error{"Unexpected wpa_supplicant response"};

  buffer[nbytes] = 0;

  return ParseScanResults(dest, max, buffer);
}

unsigned
WPASupplicant::AddNetwork()
{
  SendCommand("ADD_NETWORK");

  char buffer[4096];
  ssize_t nbytes = ReadTimeout(buffer, sizeof(buffer));
  if (nbytes < 2 || buffer[nbytes - 1] != '\n')
    throw std::runtime_error{"Unexpected wpa_supplicant response"};

  buffer[nbytes - 1] = 0;

  char *endptr;
  unsigned id = ParseUnsigned(buffer, &endptr);
  if (endptr == buffer || *endptr != 0)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  return id;
}

void
WPASupplicant::SetNetworkString(unsigned id,
                                const char *name, const char *value)
{
  NarrowString<512> cmd;
  cmd.Format("SET_NETWORK %u %s \"%s\"", id, name, value);
  SendCommand(cmd);
  ExpectOK();
}

void
WPASupplicant::SetNetworkID(unsigned id,
                                const char *name, const char *value)
{
  NarrowString<512> cmd;
  cmd.Format("SET_NETWORK %u %s %s", id, name, value);
  SendCommand(cmd);
  ExpectOK();
}

void
WPASupplicant::SelectNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("SELECT_NETWORK %u", id);
  SendCommand(cmd);
  ExpectOK();
}

void
WPASupplicant::EnableNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("ENABLE_NETWORK %u", id);
  SendCommand(cmd);
  ExpectOK();
}

void
WPASupplicant::DisableNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("DISABLE_NETWORK %u", id);
  SendCommand(cmd);
  ExpectOK();
}

void
WPASupplicant::RemoveNetwork(unsigned id)
{
  NarrowString<64> cmd;
  cmd.Format("REMOVE_NETWORK %u", id);
  SendCommand(cmd);
  ExpectOK();
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

static std::size_t
ParseListResults(WifiConfiguredNetworkInfo *dest, std::size_t max, char *src)
{
  if (memcmp(src, "network id", 10) != 0)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  src = strchr(src, '\n');
  if (src == nullptr)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  ++src;

  std::size_t n = 0;
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

std::size_t
WPASupplicant::ListNetworks(WifiConfiguredNetworkInfo *dest, std::size_t max)
{
  assert(dest != nullptr);
  assert(max > 0);

  SendCommand("LIST_NETWORKS");

  char buffer[4096];
  ssize_t nbytes = ReadTimeout(buffer, sizeof(buffer) - 1);
  if (nbytes <= 5)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  buffer[nbytes] = 0;

  return ParseListResults(dest, max, buffer);
}

void
WPASupplicant::ReadDiscard() noexcept
{
  std::byte buffer[4096];

  while (fd.Read(buffer, sizeof(buffer)) > 0) {}
}

std::size_t
WPASupplicant::ReadTimeout(void *buffer, size_t length, int timeout_ms)
{
  /* TODO: this is a kludge, because SocketDescriptor::Read()
     hard-codes MSG_DONTWAIT; we would be better off moving all of
     this into an IOLoop/IOThread */

  ssize_t nbytes = fd.Read(buffer, length);
  if (nbytes < 0) {
    const int e = errno;
    if (e != EAGAIN)
      throw MakeErrno(e, "Failed to receive response from wpa_supplicant");

    const int r = fd.WaitReadable(timeout_ms);
    if (r < 0)
      throw MakeErrno("Failed to receive response from wpa_supplicant");

    if (r == 0)
      throw std::runtime_error{"Timeout waiting for wpa_supplicant response"};

    nbytes = fd.Read(buffer, length);
  }

  if (nbytes < 0)
    throw MakeErrno("Failed to receive response from wpa_supplicant");

  if (nbytes == 0) {
    fd.Close();
    throw std::runtime_error{"Connection to wpa_supplicant closed"};
  }

  return nbytes;
}
