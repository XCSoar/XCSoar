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
#include "Device/Port.hpp"

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

bool
CAI302::CommandMode(Port &port)
{
  port.Flush();
  return CommandModeQuick(port) && port.ExpectString("cmd>");
}

bool
CAI302::LogModeQuick(Port &port)
{
  return CommandModeQuick(port) && WriteString(port, "LOG 0\r");
}

bool
CAI302::LogMode(Port &port)
{
  if (!CommandMode(port))
    return false;

  port.Flush();
  return WriteString(port, "LOG 0\r");
}

static bool
WaitUploadPrompt(Port &port)
{
  return port.ExpectString("up>");
}

bool
CAI302::UploadMode(Port &port)
{
  if (!CommandMode(port))
    return false;

  port.Flush();
  return WriteString(port, "upl 1\r") && WaitUploadPrompt(port);
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
  if (!CommandMode(port))
    return false;

  port.Flush();
  return WriteString(port, "dow 1\r") && WaitDownloadPrompt(port);
}

bool
CAI302::DownloadCommand(Port &port, const char *command, unsigned timeout_ms)
{
  return WriteString(port, command) && WaitDownloadPrompt(port);
}
