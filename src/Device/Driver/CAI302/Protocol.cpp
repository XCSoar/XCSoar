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

#include "Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "OS/ByteOrder.hpp"
#include "Geo/GeoPoint.hpp"

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <stdio.h>

bool
CAI302::WriteString(Port &port, const char *p, OperationEnvironment &env)
{
  size_t length = strlen(p);
  return port.FullWrite(p, length, env, 2000);
}

bool
CAI302::CommandModeQuick(Port &port)
{
  return port.Write('\x03');
}

static bool
WaitCommandPrompt(Port &port, OperationEnvironment &env,
                  unsigned timeout_ms=2000)
{
  return port.ExpectString("cmd>", env, timeout_ms);
}

bool
CAI302::CommandMode(Port &port, OperationEnvironment &env)
{
  port.Flush();
  return CommandModeQuick(port) && WaitCommandPrompt(port, env);
}

bool
CAI302::SendCommandQuick(Port &port, const char *cmd,
                         OperationEnvironment &env)
{
  if (!CommandMode(port, env))
    return false;

  port.Flush();
  return WriteString(port, cmd, env);
}

bool
CAI302::SendCommand(Port &port, const char *cmd,
                    OperationEnvironment &env, unsigned timeout_ms)
{
  return SendCommandQuick(port, cmd, env) &&
    WaitCommandPrompt(port, env, timeout_ms);
}

bool
CAI302::LogModeQuick(Port &port, OperationEnvironment &env)
{
  return CommandModeQuick(port) && WriteString(port, "LOG 0\r", env);
}

bool
CAI302::LogMode(Port &port, OperationEnvironment &env)
{
  return SendCommandQuick(port, "LOG 0\r", env);
}

static bool
WaitUploadPrompt(Port &port, OperationEnvironment &env,
                 unsigned timeout_ms=2000)
{
  return port.ExpectString("up>", env, timeout_ms);
}

bool
CAI302::UploadMode(Port &port, OperationEnvironment &env)
{
  return SendCommandQuick(port, "UPLOAD 1\r", env) &&
    WaitUploadPrompt(port, env);
}

int
CAI302::ReadShortReply(Port &port, void *buffer, unsigned max_size,
                       OperationEnvironment &env, unsigned timeout_ms)
{
  unsigned char header[3];
  if (!port.FullRead(header, sizeof(header), env, timeout_ms))
    return -1;

  unsigned size = header[0];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > max_size)
    size = max_size;

  if (!port.FullRead(buffer, size, env, timeout_ms))
    return -1;

  // XXX verify the checksum

  if (size < max_size) {
    /* fill the rest with zeroes */
    char *p = (char *)buffer;
    std::fill(p + size, p + max_size, 0);
  }

  return size;
}

int
CAI302::ReadLargeReply(Port &port, void *buffer, unsigned max_size,
                       OperationEnvironment &env, unsigned timeout_ms)
{
  unsigned char header[5];
  if (!port.FullRead(header, sizeof(header), env, timeout_ms))
    return -1;

  if (header[0] == 0x09 && header[1] >= 0x10 &&
      header[3] == 0x0d && header[4] == 0x0a) {
    /* this is probably a "short" reply with an upload prompt, due to
       a transmission error - now see if the remaining 4 bytes contain
       the "up>" prompt */

    char prompt[4];
    if (port.Read(prompt, 4) == 4 && prompt[0] == 0x0a &&
        prompt[1] == 'u' && prompt[2] == 'p' && prompt[3] == '>')
      return -2;

    return -1;
  }

  unsigned size = (header[0] << 8) | header[1];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > max_size)
    size = max_size;

  if (!port.FullRead(buffer, size, env, timeout_ms))
    return -1;

  // XXX verify the checksum

  if (size < max_size) {
    /* fill the rest with zeroes */
    char *p = (char *)buffer;
    std::fill(p + size, p + max_size, 0);
  }

  return size;
}

int
CAI302::UploadShort(Port &port, const char *command,
                    void *response, unsigned max_size,
                    OperationEnvironment &env, unsigned timeout_ms)
{
  port.Flush();
  if (!WriteString(port, command, env))
    return -1;

  int nbytes = ReadShortReply(port, response, max_size, env, timeout_ms);
  if (nbytes < 0)
    return nbytes;

  if (!WaitUploadPrompt(port, env))
    return -1;

  return nbytes;
}

int
CAI302::UploadLarge(Port &port, const char *command,
                    void *response, unsigned max_size,
                    OperationEnvironment &env, unsigned timeout_ms)
{
  port.Flush();
  if (!WriteString(port, command, env))
    return -1;

  int nbytes = ReadLargeReply(port, response, max_size, env, timeout_ms);

  if (nbytes == -2) {
    /* transmission error - try again */

    if (!WriteString(port, command, env))
      return -1;

    nbytes = ReadLargeReply(port, response, max_size, env, timeout_ms);
  }

  if (nbytes < 0)
    return nbytes;

  if (!WaitUploadPrompt(port, env))
    return -1;

  return nbytes;
}

bool
CAI302::UploadGeneralInfo(Port &port, GeneralInfo &data,
                          OperationEnvironment &env)
{
  return UploadShort(port, "W\r", &data, sizeof(data), env) == sizeof(data);
}

bool
CAI302::UploadFileList(Port &port, unsigned i, FileList &data,
                       OperationEnvironment &env)
{
  assert(i < 8);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 196 + i);
  return UploadLarge(port, cmd, &data, sizeof(data), env) == sizeof(data);
}

bool
CAI302::UploadFileASCII(Port &port, unsigned i, FileASCII &data,
                        OperationEnvironment &env)
{
  assert(i < 64);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 64 + i);
  return UploadLarge(port, cmd, &data, sizeof(data), env) == sizeof(data);
}

bool
CAI302::UploadFileBinary(Port &port, unsigned i, FileBinary &data,
                         OperationEnvironment &env)
{
  assert(i < 64);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 256 + i);
  return UploadLarge(port, cmd, &data, sizeof(data), env) == sizeof(data);
}

int
CAI302::UploadFileData(Port &port, bool next, void *data, unsigned length,
                       OperationEnvironment &env)
{
  return UploadLarge(port, next ? "B N\r" : "B R\r", data, length, env, 15000);
}

bool
CAI302::UploadFileSignatureASCII(Port &port, FileSignatureASCII &data,
                                 OperationEnvironment &env)
{
  return UploadLarge(port, "B S\r", &data, sizeof(data), env) == sizeof(data);
}

bool
CAI302::UploadPolarMeta(Port &port, PolarMeta &data, OperationEnvironment &env)
{
  return UploadShort(port, "G\r", &data, sizeof(data), env) > 0;
}

bool
CAI302::UploadPolar(Port &port, Polar &data, OperationEnvironment &env)
{
  return UploadShort(port, "G 0\r", &data, sizeof(data), env) > 0;
}

bool
CAI302::UploadPilotMeta(Port &port, PilotMeta &data, OperationEnvironment &env)
{
  return UploadShort(port, "O\r", &data, sizeof(data), env) > 0;
}

bool
CAI302::UploadPilotMetaActive(Port &port, PilotMetaActive &data,
                              OperationEnvironment &env)
{
  return UploadShort(port, "O A\r", &data, sizeof(data), env) > 0;
}

bool
CAI302::UploadPilot(Port &port, unsigned i, Pilot &data,
                    OperationEnvironment &env)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "O %u\r", i);
  return UploadShort(port, cmd, &data, sizeof(data), env) > 0;
}

int
CAI302::UploadPilotBlock(Port &port, unsigned start, unsigned count,
                         unsigned record_size, void *buffer,
                         OperationEnvironment &env)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "O B %u %u\r", start, count);

  /* the CAI302 data port user's guide 2.2 says that the "O B"
     response is "large", but this seems wrong */
  int nbytes = UploadShort(port, cmd, buffer, count * record_size, env);
  return nbytes >= 0 && nbytes % record_size == 0
    ? nbytes / record_size
    : -1;
}

static bool
WaitDownloadPrompt(Port &port, OperationEnvironment &env,
                   unsigned timeout_ms=2000)
{
  return port.ExpectString("dn>", env, timeout_ms);
}

bool
CAI302::DownloadMode(Port &port, OperationEnvironment &env)
{
  return SendCommandQuick(port, "DOWNLOAD 1\r", env) &&
    WaitDownloadPrompt(port, env);
}

bool
CAI302::DownloadCommand(Port &port, const char *command,
                        OperationEnvironment &env, unsigned timeout_ms)
{
  return WriteString(port, command, env) && WaitDownloadPrompt(port, env);
}

bool
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

  return DownloadCommand(port, buffer, env);
}

bool
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

  return DownloadCommand(port, buffer, env);
}

bool
CAI302::UploadNavpointMeta(Port &port, NavpointMeta &data,
                           OperationEnvironment &env)
{
  return UploadShort(port, "C\r", &data, sizeof(data), env) > 0;
}

bool
CAI302::UploadNavpoint(Port &port, unsigned i, Navpoint &data,
                       OperationEnvironment &env)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "C %u\r", i);
  return UploadShort(port, cmd, &data, sizeof(data), env) > 0;
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

bool
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
  return DownloadCommand(port, buffer, env);
}

bool
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

  return DownloadCommand(port, buffer, env);
}

bool
CAI302::DeclareSave(Port &port, OperationEnvironment &env)
{
  return DownloadCommand(port, "D,255\r", env, 5000);
}

bool
CAI302::Reboot(Port &port, OperationEnvironment &env)
{
  return SendCommandQuick(port, "SIF 0 0\r", env);
}

bool
CAI302::PowerOff(Port &port, OperationEnvironment &env)
{
  return SendCommandQuick(port, "DIE\r", env);
}

bool
CAI302::StartLogging(Port &port, OperationEnvironment &env)
{
  return SendCommand(port, "START\r", env);
}

bool
CAI302::StopLogging(Port &port, OperationEnvironment &env)
{
  return SendCommand(port, "STOP\r", env);
}

bool
CAI302::SetVolume(Port &port, unsigned volume, OperationEnvironment &env)
{
  char cmd[16];
  sprintf(cmd, "VOL %u\r", volume);
  return SendCommand(port, cmd, env);
}

bool
CAI302::ClearPoints(Port &port, OperationEnvironment &env)
{
  return SendCommand(port, "CLEAR POINTS\r", env, 5000);
}

bool
CAI302::ClearPilot(Port &port, OperationEnvironment &env)
{
  return SendCommand(port, "CLEAR PILOT\r", env, 5000);
}

bool
CAI302::ClearLog(Port &port, OperationEnvironment &env)
{
  return SendCommand(port, "CLEAR LOG\r", env, 60000);
}

static unsigned
ConvertBaudRate(unsigned baud_rate)
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

bool
CAI302::SetBaudRate(Port &port, unsigned baud_rate, OperationEnvironment &env)
{
  unsigned n = ConvertBaudRate(baud_rate);
  if (n == 0)
    return false;

  char cmd[16];
  sprintf(cmd, "BAUD %u\r", n);
  return SendCommandQuick(port, cmd, env);
}
