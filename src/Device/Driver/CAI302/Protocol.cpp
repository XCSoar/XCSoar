// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/ByteOrder.hxx"
#include "util/SpanCast.hxx"

#include <algorithm>
#include <cassert>
#include <string.h>
#include <stdio.h>

void
CAI302::WriteString(Port &port, std::string_view s, OperationEnvironment &env)
{
  port.FullWrite(s, env, std::chrono::seconds(2));
}

void
CAI302::CommandModeQuick(Port &port)
{
  port.Write('\x03');
}

static void
WaitCommandPrompt(Port &port, OperationEnvironment &env,
                  std::chrono::steady_clock::duration timeout=std::chrono::seconds(2))
{
  port.ExpectString("cmd>", env, timeout);
}

void
CAI302::CommandMode(Port &port, OperationEnvironment &env)
{
  port.Flush();
  CommandModeQuick(port);
  WaitCommandPrompt(port, env);
}

void
CAI302::SendCommandQuick(Port &port, const char *cmd,
                         OperationEnvironment &env)
{
  CommandMode(port, env);
  port.Flush();
  WriteString(port, cmd, env);
}

void
CAI302::SendCommand(Port &port, const char *cmd,
                    OperationEnvironment &env,
                    std::chrono::steady_clock::duration timeout)
{
  SendCommandQuick(port, cmd, env);
  WaitCommandPrompt(port, env, timeout);
}

void
CAI302::LogModeQuick(Port &port, OperationEnvironment &env)
{
  CommandModeQuick(port);
  WriteString(port, "LOG 0\r", env);
}

void
CAI302::LogMode(Port &port, OperationEnvironment &env)
{
  SendCommandQuick(port, "LOG 0\r", env);
}

static void
WaitUploadPrompt(Port &port, OperationEnvironment &env,
                 std::chrono::steady_clock::duration timeout=std::chrono::seconds(2))
{
  port.ExpectString("up>", env, timeout);
}

void
CAI302::UploadMode(Port &port, OperationEnvironment &env)
{
  SendCommandQuick(port, "UPLOAD 1\r", env);
  WaitUploadPrompt(port, env);
}

int
CAI302::ReadShortReply(Port &port, std::span<std::byte> dest,
                       OperationEnvironment &env,
                       std::chrono::steady_clock::duration timeout)
{
  unsigned char header[3];
  port.FullRead(std::as_writable_bytes(std::span{header}), env, timeout);

  unsigned size = header[0];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > dest.size())
    size = dest.size();

  port.FullRead(dest.first(size), env, timeout);

  // XXX verify the checksum

  /* fill the rest with zeroes */
  std::fill(std::next(dest.begin(), size), dest.end(), std::byte{});

  return size;
}

int
CAI302::ReadLargeReply(Port &port, std::span<std::byte> dest,
                       OperationEnvironment &env,
                       std::chrono::steady_clock::duration timeout)
{
  unsigned char header[5];
  port.FullRead(std::as_writable_bytes(std::span{header}), env, timeout);

  if (header[0] == 0x09 && header[1] >= 0x10 &&
      header[3] == 0x0d && header[4] == 0x0a) {
    /* this is probably a "short" reply with an upload prompt, due to
       a transmission error - now see if the remaining 4 bytes contain
       the "up>" prompt */

    char prompt[4];
    if (port.Read(std::as_writable_bytes(std::span{prompt})) == 4 &&
        prompt[0] == 0x0a && prompt[1] == 'u' &&
        prompt[2] == 'p' && prompt[3] == '>')
      return -2;

    return -1;
  }

  unsigned size = (header[0] << 8) | header[1];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > dest.size())
    size = dest.size();

  port.FullRead(dest.first(size), env, timeout);

  // XXX verify the checksum

  /* fill the rest with zeroes */
  std::fill(std::next(dest.begin(), size), dest.end(), std::byte{});

  return size;
}

int
CAI302::UploadShort(Port &port, const char *command,
                    std::span<std::byte> response,
                    OperationEnvironment &env,
                    std::chrono::steady_clock::duration timeout)
{
  port.Flush();
  WriteString(port, command, env);

  int nbytes = ReadShortReply(port, response, env, timeout);
  if (nbytes < 0)
    return nbytes;

  WaitUploadPrompt(port, env);
  return nbytes;
}

int
CAI302::UploadLarge(Port &port, const char *command,
                    std::span<std::byte> response,
                    OperationEnvironment &env,
                    std::chrono::steady_clock::duration timeout)
{
  port.Flush();
  WriteString(port, command, env);

  int nbytes = ReadLargeReply(port, response, env, timeout);

  if (nbytes == -2) {
    /* transmission error - try again */

    WriteString(port, command, env);

    nbytes = ReadLargeReply(port, response, env, timeout);
  }

  if (nbytes < 0)
    return nbytes;

  WaitUploadPrompt(port, env);
  return nbytes;
}

bool
CAI302::UploadGeneralInfo(Port &port, GeneralInfo &data,
                          OperationEnvironment &env)
{
  return UploadShort(port, "W\r",
                     ReferenceAsWritableBytes(data),
                     env) == sizeof(data);
}

bool
CAI302::UploadFileList(Port &port, unsigned i, FileList &data,
                       OperationEnvironment &env)
{
  assert(i < 8);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 196 + i);
  return UploadLarge(port, cmd,
                     ReferenceAsWritableBytes(data),
                     env) == sizeof(data);
}

bool
CAI302::UploadFileASCII(Port &port, unsigned i, FileASCII &data,
                        OperationEnvironment &env)
{
  assert(i < 64);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 64 + i);
  return UploadLarge(port, cmd,
                     ReferenceAsWritableBytes(data),
                     env) == sizeof(data);
}

bool
CAI302::UploadFileBinary(Port &port, unsigned i, FileBinary &data,
                         OperationEnvironment &env)
{
  assert(i < 64);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 256 + i);
  return UploadLarge(port, cmd,
                     ReferenceAsWritableBytes(data),
                     env) == sizeof(data);
}

int
CAI302::UploadFileData(Port &port, bool next, std::span<std::byte> dest,
                       OperationEnvironment &env)
{
  return UploadLarge(port, next ? "B N\r" : "B R\r", dest, env,
                     std::chrono::seconds(15));
}

bool
CAI302::UploadFileSignatureASCII(Port &port, FileSignatureASCII &data,
                                 OperationEnvironment &env)
{
  return UploadLarge(port, "B S\r",
                     ReferenceAsWritableBytes(data),
                     env) == sizeof(data);
}

bool
CAI302::UploadPolarMeta(Port &port, PolarMeta &data, OperationEnvironment &env)
{
  return UploadShort(port, "G\r",
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

bool
CAI302::UploadPolar(Port &port, Polar &data, OperationEnvironment &env)
{
  return UploadShort(port, "G 0\r",
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

bool
CAI302::UploadPilotMeta(Port &port, PilotMeta &data, OperationEnvironment &env)
{
  return UploadShort(port, "O\r",
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

bool
CAI302::UploadPilotMetaActive(Port &port, PilotMetaActive &data,
                              OperationEnvironment &env)
{
  return UploadShort(port, "O A\r",
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

bool
CAI302::UploadPilot(Port &port, unsigned i, Pilot &data,
                    OperationEnvironment &env)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "O %u\r", i);
  return UploadShort(port, cmd,
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

int
CAI302::UploadPilotBlock(Port &port, unsigned start, unsigned count,
                         unsigned record_size, std::byte *buffer,
                         OperationEnvironment &env)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "O B %u %u\r", start, count);

  /* the CAI302 data port user's guide 2.2 says that the "O B"
     response is "large", but this seems wrong */
  int nbytes = UploadShort(port, cmd, {buffer, count * record_size}, env);
  return nbytes >= 0 && nbytes % record_size == 0
    ? nbytes / record_size
    : -1;
}

static void
WaitDownloadPrompt(Port &port, OperationEnvironment &env,
                   std::chrono::steady_clock::duration timeout=std::chrono::seconds(2))
{
  port.ExpectString("dn>", env, timeout);
}

void
CAI302::DownloadMode(Port &port, OperationEnvironment &env)
{
  SendCommandQuick(port, "DOWNLOAD 1\r", env);
  WaitDownloadPrompt(port, env);
}

void
CAI302::DownloadCommand(Port &port, const char *command,
                        OperationEnvironment &env,
                        [[maybe_unused]] std::chrono::steady_clock::duration timeout)
{
  WriteString(port, command, env);
  WaitDownloadPrompt(port, env);
}

void
CAI302::DownloadPilot(Port &port, const Pilot &pilot, unsigned ordinal,
                      OperationEnvironment &env)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "O,%-24s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r",
           pilot.name,
           (ordinal << 8) | pilot.old_units,
           pilot.old_temperatur_units,
           pilot.sink_tone,
           pilot.total_energy_final_glide,
           pilot.show_final_glide_altitude_difference,
           pilot.map_datum,
           FromBE16(pilot.approach_radius),
           FromBE16(pilot.arrival_radius),
           FromBE16(pilot.enroute_logging_interval),
           FromBE16(pilot.close_logging_interval),
           FromBE16(pilot.time_between_flight_logs),
           FromBE16(pilot.minimum_speed_to_force_flight_logging),
           pilot.stf_dead_band,
           pilot.reserved_vario,
           FromBE16(pilot.unit_word),
           FromBE16(pilot.margin_height));

  DownloadCommand(port, buffer, env);
}

void
CAI302::DownloadPolar(Port &port, const Polar &polar,
                      OperationEnvironment &env)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "G,%-12s,%-12s,%d,%d,%d,%d,%d,%d,%d,%d\r",
           polar.glider_type,
           polar.glider_id,
           polar.best_ld,
           polar.best_glide_speed,
           polar.two_ms_sink_at_speed,
           FromBE16(polar.weight_in_litres),
           FromBE16(polar.ballast_capacity),
           0,
           FromBE16(polar.config_word),
           FromBE16(polar.wing_area));

  DownloadCommand(port, buffer, env);
}

bool
CAI302::UploadNavpointMeta(Port &port, NavpointMeta &data,
                           OperationEnvironment &env)
{
  return UploadShort(port, "C\r",
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

bool
CAI302::UploadNavpoint(Port &port, unsigned i, Navpoint &data,
                       OperationEnvironment &env)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "C %u\r", i);
  return UploadShort(port, cmd,
                     ReferenceAsWritableBytes(data),
                     env) > 0;
}

static void
FormatGeoPoint(char *buffer, const GeoPoint &location)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  tmp = (double)location.latitude.Degrees();
  NoS = 'N';
  if (tmp < 0) {
    NoS = 'S';
    tmp = -tmp;
  }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60;

  tmp = (double)location.longitude.Degrees();
  EoW = 'E';
  if (tmp < 0) {
    EoW = 'W';
    tmp = -tmp;
  }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60;

  sprintf(buffer, "%02d%07.4f%c,%03d%07.4f%c",
          DegLat, MinLat, NoS,
          DegLon, MinLon, EoW);
}

void
CAI302::DownloadNavpoint(Port &port, const GeoPoint &location,
                         int altitude, unsigned id,
                         bool turnpoint, bool airfield, bool markpoint,
                         bool landing_point, bool start_point,
                         bool finish_point, bool home_point,
                         bool thermal_point, bool waypoint, bool airspace,
                         const char *name, const char *remark,
                         OperationEnvironment &env)
{
  assert(name != nullptr);

  char location_string[32];
  FormatGeoPoint(location_string, location);

  unsigned attr = turnpoint | (airfield << 1) | (markpoint << 2) |
    (landing_point << 3) | (start_point << 4) | (finish_point << 5) |
    (home_point << 6) | (thermal_point << 7) | (waypoint << 8) |
    (airspace << 9);

  if (remark == nullptr)
    remark = "";

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "C,0,%s,%d,%u,%u,%-12s,%-12s\r",
           location_string, altitude, id, attr, name, remark);
  DownloadCommand(port, buffer, env);
}

void
CAI302::CloseNavpoints(Port &port, OperationEnvironment &env)
{
  DownloadCommand(port, "C,-1\r", env, std::chrono::seconds(5));
}

void
CAI302::DeclareTP(Port &port, unsigned i, const GeoPoint &location,
                  int altitude, const char *name, OperationEnvironment &env)
{
  char location_string[32];
  FormatGeoPoint(location_string, location);

  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "D,%d,%s,%s,%d\r",
           128 + i, location_string,
           name,
           altitude);

  DownloadCommand(port, buffer, env);
}

void
CAI302::DeclareSave(Port &port, OperationEnvironment &env)
{
  DownloadCommand(port, "D,255\r", env, std::chrono::seconds(5));
}

void
CAI302::Reboot(Port &port, OperationEnvironment &env)
{
  SendCommandQuick(port, "SIF 0 0\r", env);
}

void
CAI302::PowerOff(Port &port, OperationEnvironment &env)
{
  SendCommandQuick(port, "DIE\r", env);
}

void
CAI302::StartLogging(Port &port, OperationEnvironment &env)
{
  SendCommand(port, "START\r", env);
}

void
CAI302::StopLogging(Port &port, OperationEnvironment &env)
{
  SendCommand(port, "STOP\r", env);
}

void
CAI302::SetVolume(Port &port, unsigned volume, OperationEnvironment &env)
{
  char cmd[16];
  sprintf(cmd, "VOL %u\r", volume);
  SendCommand(port, cmd, env);
}

void
CAI302::ClearPoints(Port &port, OperationEnvironment &env)
{
  SendCommand(port, "CLEAR POINTS\r", env, std::chrono::seconds(5));
}

void
CAI302::ClearPilot(Port &port, OperationEnvironment &env)
{
  SendCommand(port, "CLEAR PILOT\r", env, std::chrono::seconds(5));
}

void
CAI302::ClearLog(Port &port, OperationEnvironment &env)
{
  SendCommand(port, "CLEAR LOG\r", env, std::chrono::minutes(1));
}

static constexpr unsigned
ConvertBaudRate(unsigned baud_rate) noexcept
{
  switch (baud_rate) {
  case 1200: return 4;
  case 2400: return 5;
  case 4800: return 6;
  case 9600: return 7;
  case 19200: return 8;
  case 38400: return 9;
  case 57600: return 10;
  case 115200: return 11;
  default: return 0;
  }
}

void
CAI302::SetBaudRate(Port &port, unsigned baud_rate, OperationEnvironment &env)
{
  unsigned n = ConvertBaudRate(baud_rate);
  if (n == 0)
    throw std::runtime_error("Baud rate not supported by CAI302");

  char cmd[20];
  sprintf(cmd, "BAUD %u\r", n);
  SendCommandQuick(port, cmd, env);
}
