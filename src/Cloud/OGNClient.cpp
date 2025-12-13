// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNClient.hpp"
#include "Dump.hpp"
#include "FLARM/Traffic.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "OGNTraffic.hpp"
#include "util/CharUtil.hxx"
#include "util/NumberParser.hxx"
#include "util/StringCompare.hxx"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;

OGNClient::OGNClient(EventLoop &_event_loop, Cares::Channel &_cares,
                     OGNTrafficContainer &_traffic_container)
    : event_loop(_event_loop), traffic_container(_traffic_container),
      cares_channel(&_cares),
      socket_event(event_loop, BIND_THIS_METHOD(OnSocketReady)),
      reconnect_timer(event_loop, BIND_THIS_METHOD(OnReconnectTimer)),
      keepalive_timer(event_loop, BIND_THIS_METHOD(OnKeepaliveTimer))
{
}

OGNClient::~OGNClient() { Close(); }

void
OGNClient::Open()
{
  if (!enabled || server == nullptr || port == 0) return;

  Close();

  // Based on python-ogn-client:
  // - Port 10152 = full feed (unfiltered)
  // - Port 14580 = filtered feeds
  // If filter is set, we should use 14580, otherwise 10152
  // But for now, we'll use the configured port and let the caller set it
  // correctly (Main.cpp sets port 10152 for unfiltered, and we'll switch to
  // 14580 when filters are added)

  resolver.emplace(*this, port);
  resolver->Start(*cares_channel, server);
}

void
OGNClient::Close()
{
  reconnect_timer.Cancel();
  keepalive_timer.Cancel();
  resolver.reset();
  connect.reset();
  socket_event.Close();
  input_buffer.clear();
  resolved_addresses.clear();
}

void
OGNClient::OnResolverSuccess(
    std::forward_list<AllocatedSocketAddress> addresses) noexcept
{
  resolver.reset();

  if (addresses.empty()) {
    cerr << "OGN: No address resolved" << endl;
    reconnect_timer.Schedule(std::chrono::seconds(30));
    return;
  }

  // Store all addresses and try them one by one
  resolved_addresses = std::move(addresses);
  current_address = resolved_addresses.begin();
  TryNextAddress();
}

void
OGNClient::TryNextAddress() noexcept
{
  if (current_address == resolved_addresses.end()) {
    // All addresses exhausted
    cerr << "OGN: All connection attempts failed" << endl;
    resolved_addresses.clear();
    reconnect_timer.Schedule(std::chrono::seconds(30));
    return;
  }

  address = *current_address;
  connect.emplace(event_loop, *this);
  connect->Connect(address, std::chrono::seconds(30));
}

void
OGNClient::OnResolverError([[maybe_unused]] std::exception_ptr error) noexcept
{
  resolver.reset();
  cerr << "OGN: Resolver error" << endl;
  reconnect_timer.Schedule(std::chrono::seconds(30));
}

void
OGNClient::OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept
{
  connect.reset();
  resolved_addresses.clear(); // Clear address list on success

  socket_event.Open(fd.Release());
  socket_event.ScheduleRead();

  cout << "OGN: Connected to " << server << ":" << port << endl;

  keepalive_timer.Schedule(std::chrono::seconds(240));
}

void
OGNClient::OnSocketConnectError(
    [[maybe_unused]] std::exception_ptr ep) noexcept
{
  connect.reset();

  // Try next address if available
  if (current_address != resolved_addresses.end()) {
    ++current_address;
    cerr << "OGN: Connection failed, trying next address..." << endl;
    TryNextAddress();
    return;
  }

  // All addresses exhausted
  cerr << "OGN: Connection error - all addresses failed" << endl;
  resolved_addresses.clear();
  reconnect_timer.Schedule(std::chrono::seconds(30));
}

void
OGNClient::OnKeepaliveTimer() noexcept
{
  if (IsConnected()) {
    // Send keepalive as per python-ogn-client
    const char *keepalive = "#keepalive\n";
    (void)socket_event.GetSocket().Write(
        {(const std::byte *)keepalive, strlen(keepalive)});
    // Reschedule for next keepalive (240 seconds)
    keepalive_timer.Schedule(std::chrono::seconds(240));
  }
}

void
OGNClient::OnReconnectTimer() noexcept
{
  if (enabled && !IsConnected()) Open();
}

void
OGNClient::OnSocketReady([[maybe_unused]] unsigned events) noexcept
{
  std::byte buffer[4096];
  ssize_t nbytes = socket_event.GetSocket().Read({buffer, sizeof(buffer)});

  if (nbytes < 0) {
    cerr << "OGN: Read error" << endl;
    Close();
    reconnect_timer.Schedule(std::chrono::seconds(30));
    return;
  }

  if (nbytes == 0) {
    cerr << "OGN: Connection closed" << endl;
    Close();
    reconnect_timer.Schedule(std::chrono::seconds(30));
    return;
  }

  // Append to input buffer
  input_buffer.append((const char *)buffer, nbytes);

  // Process complete lines
  for (;;) {
    size_t pos = input_buffer.find('\n');
    if (pos == std::string::npos) break;

    std::string line = input_buffer.substr(0, pos);
    input_buffer.erase(0, pos + 1);

    // Remove \r if present
    if (!line.empty() && line.back() == '\r') line.pop_back();

    // Handle zero-length lines (connection failure indicator)
    // Per python-ogn-client: "A zero length line should not be returned if
    // keepalives are being sent"
    if (line.empty()) {
      cerr << "OGN: Zero-length line received - connection failure" << endl;
      Close();
      reconnect_timer.Schedule(std::chrono::seconds(30));
      return;
    }

    if (!line.empty() && line[0] != '#') {
      ProcessLine(line.c_str());
    }
  }

  // Keep reading from socket
  socket_event.ScheduleRead();
}

void
OGNClient::ProcessLine(const char *line)
{
  // APRS-IS format: "callsign>destination,path:data"
  const char *colon = strchr(line, ':');
  if (colon == nullptr) {
    // Silently skip lines without colon (not a valid APRS packet)
    return;
  }

  // Extract callsign (before >) - this is the OGN device ID
  const char *gt = strchr(line, '>');
  if (gt == nullptr || gt >= colon) {
    // Silently skip lines without > (not a valid APRS packet)
    return;
  }

  tstring device_id(line, gt - line);

  // Debug: log all beacons received
  cout << "OGN: Received line from " << device_id.c_str() << ": " << line
       << endl;

  // Skip receiver beacons (they're not aircraft traffic)
  // Receiver beacons have format: "LKHS>APRS,TCPIP*,qAC,GLIDERN2:/..."
  // They contain ">APRS,TCPIP*" or "qAC" in the path
  const char *path_start = gt + 1;
  const char *path_end = colon;
  if (path_end > path_start) {
    std::string path(path_start, path_end - path_start);
    if (path.find("APRS,TCPIP*") != std::string::npos ||
        path.find("qAC") != std::string::npos) {
      // This is a receiver beacon, skip it silently
      return;
    }
  }

  // Since we're using a bounding box filter, the server should already
  // filter by location. Try to parse ALL position packets we receive.
  // We'll identify OGN traffic by the packet content rather than device ID.
  const char *packet = colon + 1;

  // Only process position packets (APRS position reports start with !, /, or
  // =)
  if (packet[0] == '!' || packet[0] == '/' || packet[0] == '=') {
    // Debug: log all position packets being processed
    cout << "OGN: Processing position packet from " << device_id.c_str()
         << ": " << packet << endl;
    // Try to parse as OGN position packet
    ParseAPRSPacket(packet, device_id);
  } else {
    // Debug: log non-position packets
    cout << "OGN: Skipping non-position packet from " << device_id.c_str()
         << " (starts with '" << packet[0] << "')" << endl;
  }
}

void
OGNClient::ParseAPRSPacket(const char *packet, const tstring &device_id)
{
  // Look for OGN position packets
  // Format: "!ddmm.hhN/dddmm.hhW.../A=aaaaaa ..."
  // or: "/ddmm.hhN/dddmm.hhW.../A=aaaaaa ..."

  if (packet[0] != '!' && packet[0] != '/' && packet[0] != '=') return;

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  if (!ParseOGNPosition(packet, location, altitude, track, speed, climb_rate,
                        turn_rate, aircraft_type, track_received)) {
    // Debug: log parsing failures (increased limit to see more)
    static int parse_fail_count = 0;
    if (parse_fail_count++ < 20) {
      cout << "OGN: Failed to parse position from " << device_id.c_str()
           << ": " << packet << endl;
    }
    return;
  }

  if (!location.IsValid()) {
    static int invalid_count = 0;
    if (invalid_count++ < 5) {
      cout << "OGN: Invalid location from " << device_id.c_str() << endl;
    }
    return;
  }

  traffic_container.Make(device_id, location, altitude, track, speed,
                         climb_rate, turn_rate, aircraft_type);

  // Debug: log all successful storage
  cout << "OGN: Stored traffic: " << device_id.c_str() << " at " << location
       << " alt=" << altitude << "m"
       << " track=" << track << "°"
       << " speed=" << speed << "m/s"
       << " climb=" << climb_rate << "m/s"
       << " turn=" << turn_rate << "deg/s"
       << " type=" << (unsigned)aircraft_type << endl;
}

static unsigned
HexDigitValue(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

bool
OGNClient::ParseOGNPosition(const char *packet, GeoPoint &location,
                            int &altitude, unsigned &track, unsigned &speed,
                            int &climb_rate, double &turn_rate,
                            FlarmTraffic::AircraftType &aircraft_type,
                            bool &track_received)
{
  // Parse latitude: ddmm.hhN or ddmm.hhS
  if (packet[0] != '!' && packet[0] != '/' && packet[0] != '=') return false;

  const char *p = packet + 1;

  // Skip timestamp if present (hhmmss)
  if (IsDigitASCII(p[0]) && IsDigitASCII(p[1]) && IsDigitASCII(p[2]) &&
      IsDigitASCII(p[3]) && IsDigitASCII(p[4]) && IsDigitASCII(p[5])) {
    p += 6;
    if (*p == 'h' || *p == 'z' || *p == '/') p++;
  }

  // Parse latitude ddmm.hh
  if (!IsDigitASCII(p[0]) || !IsDigitASCII(p[1])) return false;

  int lat_deg = (p[0] - '0') * 10 + (p[1] - '0');
  p += 2;

  if (!IsDigitASCII(p[0]) || !IsDigitASCII(p[1])) return false;

  int lat_min = (p[0] - '0') * 10 + (p[1] - '0');
  p += 2;

  if (*p == '.') {
    p++;
    if (IsDigitASCII(p[0]) && IsDigitASCII(p[1])) {
      int lat_hundredths = (p[0] - '0') * 10 + (p[1] - '0');
      lat_min = lat_min * 100 + lat_hundredths;
      p += 2;
    }
  }

  char lat_dir = *p++;
  if (lat_dir != 'N' && lat_dir != 'S') return false;

  double latitude = lat_deg + lat_min / 6000.0;
  if (lat_dir == 'S') latitude = -latitude;

  // Parse symbol to determine aircraft type
  // OGN uses various symbols: /, \, ', ^, etc. depending on aircraft type
  char symbol = *p;
  switch (symbol) {
  case '/':
    aircraft_type = FlarmTraffic::AircraftType::GLIDER;
    break;
  case '\\':
    aircraft_type = FlarmTraffic::AircraftType::POWERED_AIRCRAFT;
    break;
  case '\'':
    aircraft_type = FlarmTraffic::AircraftType::HANG_GLIDER;
    break;
  case '^':
    aircraft_type = FlarmTraffic::AircraftType::PARA_GLIDER;
    break;
  case 'O':
    aircraft_type = FlarmTraffic::AircraftType::BALLOON;
    break;
  case 'g':
  case 'z':
    aircraft_type = FlarmTraffic::AircraftType::UAV;
    break;
  default:
    aircraft_type = FlarmTraffic::AircraftType::UNKNOWN;
    break;
  }

  if (*p == '/' || *p == '\\' || *p == '\'' || *p == '^' || *p == 'X' ||
      *p == 'g' || *p == 'O' || *p == 'z' || *p == 'n')
    p++;

  // Parse longitude dddmm.hh
  if (!IsDigitASCII(p[0]) || !IsDigitASCII(p[1]) || !IsDigitASCII(p[2]))
    return false;

  int lon_deg = (p[0] - '0') * 100 + (p[1] - '0') * 10 + (p[2] - '0');
  p += 3;

  if (!IsDigitASCII(p[0]) || !IsDigitASCII(p[1])) return false;

  int lon_min = (p[0] - '0') * 10 + (p[1] - '0');
  p += 2;

  if (*p == '.') {
    p++;
    if (IsDigitASCII(p[0]) && IsDigitASCII(p[1])) {
      int lon_hundredths = (p[0] - '0') * 10 + (p[1] - '0');
      lon_min = lon_min * 100 + lon_hundredths;
      p += 2;
    }
  }

  char lon_dir = *p++;
  if (lon_dir != 'E' && lon_dir != 'W') return false;

  double longitude = lon_deg + lon_min / 6000.0;
  if (lon_dir == 'W') longitude = -longitude;

  location = GeoPoint(Angle::Degrees(longitude), Angle::Degrees(latitude));

  // Parse course/speed: format is 'ttt/sss or 'ttt/ss where ' is the symbol
  // The symbol after longitude can be various characters depending on aircraft
  // type In OGN packets, ^ and & are also used as course/speed separators
  track_received = false; // Initialize to false, set to true if track is found
  if ((*p == '\'' || *p == '/' || *p == '\\' || *p == '^' || *p == '&') &&
      IsDigitASCII(p[1]) && IsDigitASCII(p[2]) && IsDigitASCII(p[3])) {
    // Course in degrees
    unsigned parsed_track =
        (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
    if (parsed_track <= 359) {
      track = parsed_track;
      track_received = true; // Track was successfully parsed from packet
    }
    p += 4;
    if (*p == '/') {
      p++;
      if (IsDigitASCII(p[0]) && IsDigitASCII(p[1])) {
        // Speed in knots (2 or 3 digits)
        speed = (p[0] - '0') * 10 + (p[1] - '0');
        p += 2;
        if (IsDigitASCII(p[0])) {
          speed = speed * 10 + (p[0] - '0');
          p++;
        }
        // Convert knots to m/s (1 knot = 0.514444 m/s)
        speed = (unsigned)(speed * 0.514444 + 0.5);
      }
    }
  }

  // Parse additional data: /A=aaaaaa (altitude), OGN-specific fields, etc.
  while (*p != '\0' && *p != '\r' && *p != '\n') {
    if (*p == '/' && p[1] == 'A' && p[2] == '=') {
      // Altitude in feet
      p += 3;
      char *end;
      long alt_ft = strtol(p, &end, 10);
      if (end > p) {
        altitude = (int)(alt_ft * 0.3048); // Convert to meters
        p = end;
        continue;
      }
    } else if (*p == ' ' && p[1] == 'i' && p[2] == 'd') {
      // OGN-specific: " id0ADDE626" - extract device ID from id field
      // Format: idXXYYYYYY where XX encodes flags and YYYYYY is the address
      p += 3; // Skip " id"
      // The device ID is already extracted from the callsign field,
      // but we can validate/update it here if needed
      // Skip the id field (8 hex digits)
      int hex_count = 0;
      while (hex_count < 8 &&
             ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') ||
              (*p >= 'a' && *p <= 'f'))) {
        p++;
        hex_count++;
      }
      continue;
    } else if (*p == ' ' &&
               ((p[1] == '-' || p[1] == '+') && IsDigitASCII(p[2]) &&
                IsDigitASCII(p[3]) && IsDigitASCII(p[4]))) {
      // OGN-specific climb rate: " -019fpm" or " +123fpm"
      // Format: [+-]ddddfpm (feet per minute)
      bool negative = (p[1] == '-');
      p += 2; // Skip " -" or " +"
      char *end;
      long fpm = strtol(p, &end, 10);
      if (end > p && end[0] == 'f' && end[1] == 'p' && end[2] == 'm') {
        // Convert feet per minute to meters per second
        // 1 fpm = 0.00508 m/s
        climb_rate = (int)((negative ? -fpm : fpm) * 0.00508);
        p = end + 3; // Skip "fpm"
        continue;
      }
      p -= 2; // Restore if parsing failed
    } else if (*p == ' ' && p[1] == 'r' && p[2] == 'o' && p[3] == 't') {
      // OGN-specific turn rate: " rot=+12.5" or " rot=-5.0"
      // Format: rot=[+-]d.d (degrees per second)
      p += 4; // Skip " rot"
      if (*p == '=') {
        p++;
        bool negative = (*p == '-');
        if (negative || *p == '+') p++;
        char *end;
        double rot_value = strtod(p, &end);
        if (end > p) {
          turn_rate = negative ? -rot_value : rot_value;
          p = end;
          continue;
        }
      }
      p -= 4; // Restore if parsing failed
    } else if (*p == ' ' && p[1] == 'c' && p[2] == 'l' && p[3] == 'b' &&
               p[4] == '=') {
      // Alternative climb rate format: " clb=+123 " or " clb=-45 "
      p += 5;
      bool negative = (*p == '-');
      if (negative || *p == '+') p++;
      char *end;
      long clb = strtol(p, &end, 10);
      if (end > p) {
        // Assume it's in fpm if no unit, or m/s if specified
        climb_rate = negative ? -clb : clb;
        p = end;
      }
      continue;
    } else if (*p == ' ' && p[1] == 'i' && p[2] == 'd') {
      // OGN-specific: " id0ADDE626" - extract device ID and aircraft type
      // flags Format: idXXYYYYYY where XX encodes flags (including aircraft
      // type)
      p += 3; // Skip " id"
      // Parse first two hex digits for flags
      if (IsHexDigit(p[0]) && IsHexDigit(p[1])) {
        unsigned flags = (HexDigitValue(p[0]) << 4) | HexDigitValue(p[1]);
        // Bit 0-3: aircraft type (if available in flags)
        // For now, we'll rely on symbol, but could decode from flags here
        (void)flags; // Suppress unused variable warning
        p += 2;
      }
      // Skip remaining hex digits (address)
      int hex_count = 2;
      while (hex_count < 8 && IsHexDigit(*p)) {
        p++;
        hex_count++;
      }
      continue;
    }

    p++;
  }

  return true;
}
