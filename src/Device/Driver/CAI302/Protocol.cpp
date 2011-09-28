/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Engine/Navigation/GeoPoint.hpp"

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <stdio.h>

bool
CAI302::WriteString(Port &port, const char *p)
{
  size_t length = strlen(p);
  return port.FullWrite(p, length, 2000);
}

bool
CAI302::CommandModeQuick(Port &port)
{
  return port.Write('\x03');
}

static bool
WaitCommandPrompt(Port &port)
{
  return port.ExpectString("cmd>");
}

bool
CAI302::CommandMode(Port &port)
{
  port.Flush();
  return CommandModeQuick(port) && WaitCommandPrompt(port);
}

bool
CAI302::SendCommandQuick(Port &port, const char *cmd)
{
  if (!CommandMode(port))
    return false;

  port.Flush();
  return WriteString(port, cmd);
}

bool
CAI302::SendCommand(Port &port, const char *cmd)
{
  return SendCommandQuick(port, cmd) && WaitCommandPrompt(port);
}

bool
CAI302::LogModeQuick(Port &port)
{
  return CommandModeQuick(port) && WriteString(port, "LOG 0\r");
}

bool
CAI302::LogMode(Port &port)
{
  return SendCommandQuick(port, "LOG 0\r");
}

static bool
WaitUploadPrompt(Port &port)
{
  return port.ExpectString("up>");
}

bool
CAI302::UploadMode(Port &port)
{
  return SendCommandQuick(port, "UPLOAD 1\r") && WaitUploadPrompt(port);
}

int
CAI302::ReadShortReply(Port &port, void *buffer, unsigned max_size,
                       unsigned timeout_ms)
{
  unsigned char header[3];
  if (!port.FullRead(header, sizeof(header), timeout_ms))
    return -1;

  unsigned size = header[0];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > max_size)
    size = max_size;

  if (!port.FullRead(buffer, size, timeout_ms))
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
                       unsigned timeout_ms)
{
  unsigned char header[5];
  if (!port.FullRead(header, sizeof(header), timeout_ms))
    return -1;

  unsigned size = (header[0] << 8) | header[1];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > max_size)
    size = max_size;

  if (!port.FullRead(buffer, size, timeout_ms))
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
                    unsigned timeout_ms)
{
  port.Flush();
  if (!WriteString(port, command))
    return -1;

  int nbytes = ReadShortReply(port, response, max_size, timeout_ms);
  if (nbytes < 0)
    return nbytes;

  if (!WaitUploadPrompt(port))
    return -1;

  return nbytes;
}

int
CAI302::UploadLarge(Port &port, const char *command,
                    void *response, unsigned max_size,
                    unsigned timeout_ms)
{
  port.Flush();
  if (!WriteString(port, command))
    return -1;

  int nbytes = ReadLargeReply(port, response, max_size, timeout_ms);
  if (nbytes < 0)
    return nbytes;

  if (!WaitUploadPrompt(port))
    return -1;

  return nbytes;
}

bool
CAI302::UploadFileList(Port &port, unsigned i, FileList &data)
{
  assert(i < 8);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 196 + i);
  return UploadLarge(port, cmd, &data, sizeof(data)) == sizeof(data);
}

bool
CAI302::UploadFileASCII(Port &port, unsigned i, FileASCII &data)
{
  assert(i < 64);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 64 + i);
  return UploadLarge(port, cmd, &data, sizeof(data)) == sizeof(data);
}

bool
CAI302::UploadFileBinary(Port &port, unsigned i, FileBinary &data)
{
  assert(i < 64);

  char cmd[16];
  snprintf(cmd, sizeof(cmd), "B %u\r", 256 + i);
  return UploadLarge(port, cmd, &data, sizeof(data)) == sizeof(data);
}

int
CAI302::UploadFileData(Port &port, bool next, void *data, unsigned length)
{
  return UploadLarge(port, next ? "B N\r" : "B R\r", data, length, 15000);
}

bool
CAI302::UploadFileSignatureASCII(Port &port, FileSignatureASCII &data)
{
  return UploadLarge(port, "B S\r", &data, sizeof(data)) == sizeof(data);
}

bool
CAI302::UploadPolarMeta(Port &port, PolarMeta &data)
{
  return UploadShort(port, "G\r", &data, sizeof(data)) > 0;
}

bool
CAI302::UploadPolar(Port &port, Polar &data)
{
  return UploadShort(port, "G 0\r", &data, sizeof(data)) > 0;
}

bool
CAI302::UploadPilotMeta(Port &port, PilotMeta &data)
{
  return UploadShort(port, "O\r", &data, sizeof(data)) > 0;
}

bool
CAI302::UploadPilot(Port &port, unsigned i, Pilot &data)
{
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "O %u\r", i);
  return UploadShort(port, cmd, &data, sizeof(data)) > 0;
}

static bool
WaitDownloadPrompt(Port &port)
{
  return port.ExpectString("dn>");
}

bool
CAI302::DownloadMode(Port &port)
{
  return SendCommandQuick(port, "DOWNLOAD 1\r") && WaitDownloadPrompt(port);
}

bool
CAI302::DownloadCommand(Port &port, const char *command, unsigned timeout_ms)
{
  return WriteString(port, command) && WaitDownloadPrompt(port);
}

bool
CAI302::DownloadPilot(Port &port, const Pilot &pilot)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "O,%-24s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r",
           pilot.name,
           pilot.old_units,
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

  return DownloadCommand(port, buffer);
}

bool
CAI302::DownloadPolar(Port &port, const Polar &polar)
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

  return DownloadCommand(port, buffer);
}

bool
CAI302::DeclareTP(Port &port, unsigned i, const GeoPoint &location,
                  int altitude, const char *name)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  tmp = location.latitude.Degrees();
  NoS = 'N';
  if (tmp < 0) {
    NoS = 'S';
    tmp = -tmp;
  }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60;

  tmp = location.longitude.Degrees();
  EoW = 'E';
  if (tmp < 0) {
    EoW = 'W';
    tmp = -tmp;
  }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60;

  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "D,%d,%02d%07.4f%c,%03d%07.4f%c,%s,%d\r",
           128 + i,
           DegLat, MinLat, NoS,
           DegLon, MinLon, EoW,
           name,
           altitude);

  return DownloadCommand(port, buffer);
}

bool
CAI302::DeclareSave(Port &port)
{
  return DownloadCommand(port, "D,255\r");
}

bool
CAI302::Reboot(Port &port)
{
  return SendCommandQuick(port, "SIF 0 0\r");
}

bool
CAI302::PowerOff(Port &port)
{
  return SendCommandQuick(port, "DIE\r");
}

bool
CAI302::StartLogging(Port &port)
{
  return SendCommand(port, "START\r");
}

bool
CAI302::StopLogging(Port &port)
{
  return SendCommand(port, "STOP\r");
}

bool
CAI302::SetVolume(Port &port, unsigned volume)
{
  char cmd[16];
  sprintf(cmd, "VOL %u\r", volume);
  return SendCommand(port, cmd);
}

bool
CAI302::ClearPoints(Port &port)
{
  return SendCommand(port, "CLEAR POINTS\r");
}

bool
CAI302::ClearPilot(Port &port)
{
  return SendCommand(port, "CLEAR PILOT\r");
}

bool
CAI302::ClearLog(Port &port)
{
  return SendCommand(port, "CLEAR LOG\r");
}
