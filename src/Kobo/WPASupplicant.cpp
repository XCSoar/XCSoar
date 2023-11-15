// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WPASupplicant.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "lib/fmt/SystemError.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "util/IterableSplitString.hxx"
#include "util/NumberParser.hxx"
#include "util/SpanCast.hxx"
#include "util/StringSplit.hxx"

#include <stdlib.h>
#include <errno.h>
#include <time.h>

using std::string_view_literals::operator""sv;

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
    throw FmtErrno("Failed to connect to {}", path);
}

void
WPASupplicant::Close() noexcept
{
  if (fd.IsDefined())
    fd.Close();
}

void
WPASupplicant::SendCommand(std::string_view cmd)
{
  /* discard any previous responses that may be left in the socket's
     receive queue, maybe because the last command failed */
  ReadDiscard();

  const ssize_t nbytes = fd.Write(AsBytes(cmd));
  if (nbytes < 0)
    throw MakeErrno("Failed to send command to wpa_supplicant");

  if (std::size_t(nbytes) != cmd.size())
    throw std::runtime_error("Short send to wpa_supplicant");
}

void
WPASupplicant::ExpectResponse(std::string_view expected)
{
  char buffer[4096];
  assert(expected.size() <= sizeof(buffer));

  if (ReadStringTimeout(buffer) != expected)
    throw std::runtime_error{"Unexpected wpa_supplicant response"};
}

static bool
ParseStatusLine(WifiStatus &status, std::string_view src) noexcept
{
  const auto [name, value] = Split(src, '=');
  if (value.data() == nullptr)
    return false;

  if (name == "bssid"sv)
    status.bssid = value;
  else if (name == "ssid"sv)
    status.ssid = value;
  return true;
}

static bool
ParseStatus(WifiStatus &status, std::string_view src) noexcept
{
  status.Clear();

  for (const auto line : IterableSplitString(src, '\n'))
    ParseStatusLine(status, line);

  return true;
}

bool
WPASupplicant::Status(WifiStatus &status)
{
  SendCommand("STATUS");

  char buffer[4096];
  const auto src = ReadStringTimeout(buffer);
  if (src.empty())
    throw std::runtime_error{"wpa_supplicant closed the socket"};

  return ParseStatus(status, src);
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
ParseScanResultsLine(WifiVisibleNetwork &dest, std::string_view line) noexcept
{
  const auto [bssid, rest1] = Split(line, '\t');
  const auto [frequency, rest2] = Split(rest1, '\t');
  const auto [signal_level, rest3] = Split(rest2, '\t');
  const auto [flags, rest4] = Split(rest3, '\t');
  const auto [ssid, _] = Split(rest4, '\t');

  if (bssid.empty() || frequency.empty() || signal_level.empty() || ssid.empty())
    return false;

  dest.bssid = bssid;

  if (const auto value = ParseInteger<unsigned>(signal_level))
    dest.signal_level = *value;
  else
    return false;

  if (flags.find("WPA"sv) != flags.npos)
    dest.security = WPA_SECURITY;
  else if (flags.find("WEP"sv) != flags.npos)
    dest.security = WEP_SECURITY;
  else
    dest.security = OPEN_SECURITY;

  dest.ssid = ssid;
  return true;
}

static std::size_t
ParseScanResults(WifiVisibleNetwork *dest, std::size_t max, std::string_view src)
{
  if (!src.starts_with("bssid"sv))
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  src = Split(src, '\n').second;
  if (src.data() == nullptr)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  std::size_t n = 0;
  for (const auto line : IterableSplitString(src, '\n')) {
    if (line.empty())
      break;

    if (!ParseScanResultsLine(dest[n], line))
      break;

    // skip hidden ssid
    if (!dest[n].ssid.empty()) {
      ++n;

      if (n >= max)
        break;
    }
  }

  return n;
}

std::size_t
WPASupplicant::ScanResults(WifiVisibleNetwork *dest, unsigned max)
{
  assert(dest != nullptr);
  assert(max > 0);

  SendCommand("SCAN_RESULTS");

  char buffer[4096];
  const auto src = ReadStringTimeout(buffer);
  if (src.empty())
    throw std::runtime_error{"wpa_supplicant closed the socket"};

  return ParseScanResults(dest, max, src);
}

unsigned
WPASupplicant::AddNetwork()
{
  SendCommand("ADD_NETWORK");

  char buffer[4096];
  const auto line = ExpectLineTimeout(buffer);

  if (auto id = ParseInteger<unsigned>(line))
    return *id;

  throw std::runtime_error{"Malformed wpa_supplicant response"};
}

void
WPASupplicant::SetNetworkString(unsigned id,
                                const char *name, const char *value)
{
  SendCommand(FmtBuffer<512>("SET_NETWORK {} {} \"{}\"", id, name, value).c_str());
  ExpectOK();
}

void
WPASupplicant::SetNetworkID(unsigned id,
                                const char *name, const char *value)
{
  SendCommand(FmtBuffer<512>("SET_NETWORK {} {} {}", id, name, value).c_str());
  ExpectOK();
}

void
WPASupplicant::SelectNetwork(unsigned id)
{
  SendCommand(FmtBuffer<64>("SELECT_NETWORK {}", id).c_str());
  ExpectOK();
}

void
WPASupplicant::EnableNetwork(unsigned id)
{
  SendCommand(FmtBuffer<64>("ENABLE_NETWORK {}", id).c_str());
  ExpectOK();
}

void
WPASupplicant::DisableNetwork(unsigned id)
{
  SendCommand(FmtBuffer<64>("DISABLE_NETWORK {}", id).c_str());
  ExpectOK();
}

void
WPASupplicant::RemoveNetwork(unsigned id)
{
  SendCommand(FmtBuffer<64>("REMOVE_NETWORK {}", id).c_str());
  ExpectOK();
}

static bool
ParseListResultsLine(WifiConfiguredNetworkInfo &dest, std::string_view line)
{
  const auto [id, rest1] = Split(line, '\t');
  const auto [ssid, rest2] = Split(rest1, '\t');
  const auto [bssid, _] = Split(rest2, '\t');

  if (ssid.data() == nullptr || bssid.data() == nullptr)
    return false;

  if (const auto value = ParseInteger<unsigned>(id))
    dest.id = *value;
  else
    return false;

  dest.ssid = ssid;
  dest.bssid = bssid;
  return true;
}

static std::size_t
ParseListResults(WifiConfiguredNetworkInfo *dest, std::size_t max, std::string_view src)
{
  if (!src.starts_with("network id"sv))
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  src = Split(src, '\n').second;
  if (src.data() == nullptr)
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  std::size_t n = 0;
  for (const auto line : IterableSplitString(src, '\n')) {
    if (line.empty())
      break;

    if (!ParseListResultsLine(dest[n], line))
      break;

    ++n;
    if (n >= max)
      break;
  }

  return n;
}

std::size_t
WPASupplicant::ListNetworks(WifiConfiguredNetworkInfo *dest, std::size_t max)
{
  assert(dest != nullptr);
  assert(max > 0);

  SendCommand("LIST_NETWORKS");

  char buffer[4096];
  const auto src = ReadStringTimeout(buffer);
  if (src.empty())
    throw std::runtime_error{"Malformed wpa_supplicant response"};

  return ParseListResults(dest, max, src);
}

void
WPASupplicant::ReadDiscard() noexcept
{
  std::byte buffer[4096];

  while (fd.ReadNoWait(buffer) > 0) {}
}

std::size_t
WPASupplicant::ReadTimeout(std::span<std::byte> dest, int timeout_ms)
{
  /* TODO: this is a kludge, because SocketDescriptor::Read()
     hard-codes MSG_DONTWAIT; we would be better off moving all of
     this into an IOLoop/IOThread */

  ssize_t nbytes = fd.ReadNoWait(dest);
  if (nbytes < 0) {
    const int e = errno;
    if (e != EAGAIN)
      throw MakeErrno(e, "Failed to receive response from wpa_supplicant");

    const int r = fd.WaitReadable(timeout_ms);
    if (r < 0)
      throw MakeErrno("Failed to receive response from wpa_supplicant");

    if (r == 0)
      throw std::runtime_error{"Timeout waiting for wpa_supplicant response"};

    nbytes = fd.Read(dest);
  }

  if (nbytes < 0)
    throw MakeErrno("Failed to receive response from wpa_supplicant");

  if (nbytes == 0) {
    fd.Close();
    throw std::runtime_error{"Connection to wpa_supplicant closed"};
  }

  return nbytes;
}

std::string_view
WPASupplicant::ReadStringTimeout(std::span<char> buffer, int timeout_ms)
{
  std::size_t nbytes = ReadTimeout(std::as_writable_bytes(buffer), timeout_ms);
  return ToStringView(buffer.first(nbytes));
}

std::string_view
WPASupplicant::ExpectLineTimeout(std::span<char> buffer, int timeout_ms)
{
  std::string_view result = ReadStringTimeout(buffer, timeout_ms);
  if (!result.ends_with('\n'))
    throw std::runtime_error{"Unexpected wpa_supplicant response"};

  result.remove_suffix(1);
  return result;
}
