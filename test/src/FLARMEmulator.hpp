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

#ifndef XCSOAR_FLARM_EMULATOR_HPP
#define XCSOAR_FLARM_EMULATOR_HPP

#include "DeviceEmulator.hpp"
#include "Device/Util/LineSplitter.hpp"
#include "Device/Driver/FLARM/BinaryProtocol.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Util/Macros.hpp"
#include "Util/StaticFifoBuffer.hxx"
#include "Util/StaticString.hxx"

#include <string>
#include <map>
#include <stdio.h>
#include <string.h>

class FLARMEmulator : public Emulator, PortLineSplitter {
  std::map<std::string, std::string> settings;

  bool binary;
  StaticFifoBuffer<char, 256u> binary_buffer;

public:
  FLARMEmulator():binary(false) {
    handler = this;
  }

private:
  void PFLAC_S(NMEAInputLine &line) {
    char name[64];
    line.Read(name, ARRAY_SIZE(name));

    const auto value = line.Rest();
    NarrowString<256> value_buffer;
    value_buffer.SetASCII(value.begin(), value.end());

    settings[name] = value_buffer;

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%s,%s", name,
             value_buffer.c_str());
    PortWriteNMEA(*port, buffer, *env);
  }

  void PFLAC_R(NMEAInputLine &line) {
    char name[64];
    line.Read(name, ARRAY_SIZE(name));

    auto i = settings.find(name);
    if (i == settings.end())
      return;

    const char *value = i->second.c_str();

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%s,%s", name, value);
    PortWriteNMEA(*port, buffer, *env);
  }

  void PFLAC(NMEAInputLine &line) {
    char command[4];
    line.Read(command, ARRAY_SIZE(command));

    if (strcmp(command, "S") == 0)
      PFLAC_S(line);
    else if (strcmp(command, "R") == 0)
      PFLAC_R(line);
  }

  void PFLAX() {
    binary = true;
    binary_buffer.Clear();
  }

  size_t Unescape(const uint8_t *const data, const uint8_t *const end,
                  void *_dest, size_t length) {
    uint8_t *dest = (uint8_t *)_dest;

    const uint8_t *p;
    for (p = data; length > 0; ++p) {
      if (p >= end)
        return std::min(p - data, end - data - 1);

      if (*p == FLARM::START_FRAME)
        return std::min(p - data, end - data - 1);

      if (*p == FLARM::ESCAPE) {
        ++p;
        if (p >= end)
          return std::min(p - data, end - data - 1);

        if (*p == FLARM::ESCAPE_START)
          *dest++ = FLARM::START_FRAME;
        else if (*p == FLARM::ESCAPE_ESCAPE)
          *dest++ = FLARM::ESCAPE;
        else
          return std::min(p - data, end - data - 1);
      } else
        *dest++ = *p;
    }

    return p - data;
  }

  bool SendACK(uint16_t sequence_number) {
    uint16_t payload = ToLE16(sequence_number);
    FLARM::FrameHeader header =
      FLARM::PrepareFrameHeader(sequence_number, FLARM::MT_ACK,
                                &payload, sizeof(payload));
    return port->Write(FLARM::START_FRAME) &&
      FLARM::SendEscaped(*port, &header, sizeof(header), *env, 2000) &&
      FLARM::SendEscaped(*port, &payload, sizeof(payload), *env, 2000);
  }

  size_t HandleBinary(const void *_data, size_t length) {
    const uint8_t *const data = (const uint8_t *)_data, *end = data + length;
    const uint8_t *p = data;

    p = std::find(p, end, FLARM::START_FRAME);
    if (p == NULL)
      return length;

    ++p;

    FLARM::FrameHeader header;
    size_t nbytes = Unescape(p, end, &header, sizeof(header));
    p += nbytes;
    if (nbytes < sizeof(header))
      return p - data;

    //XXX size_t payload_length = header.GetLength();

    switch (header.type) {
    case FLARM::MT_PING:
    case FLARM::MT_SELECTRECORD:
      SendACK(header.sequence_number);
      break;

    case FLARM::MT_EXIT:
      SendACK(header.sequence_number);
      binary = false;
      break;
    }

    return p - data;
  }

  void BinaryReceived(const void *_data, size_t length) {
    const uint8_t *data = (const uint8_t *)_data, *end = data + length;

    do {
      /* append new data to buffer, as much as fits there */
      auto range = binary_buffer.Write();
      if (range.IsEmpty()) {
        /* overflow: reset buffer to recover quickly */
        binary_buffer.Clear();
        continue;
      }

      size_t nbytes = std::min(size_t(range.size), size_t(end - data));
      memcpy(range.data, data, nbytes);
      data += nbytes;
      binary_buffer.Append(nbytes);

      while (true) {
        range = binary_buffer.Read();
        if (range.IsEmpty())
          break;

        size_t nbytes = HandleBinary(range.data, range.size);
        if (nbytes == 0) {
          if (binary_buffer.IsFull())
            binary_buffer.Clear();
          break;
        }

        binary_buffer.Consume(nbytes);
      }
    } while (data < end);
  }

protected:
  virtual void DataReceived(const void *data, size_t length) {
    if (binary) {
      BinaryReceived(data, length);
    } else {
      fwrite(data, 1, length, stdout);
      PortLineSplitter::DataReceived(data, length);
    }
  }

  virtual void LineReceived(const char *_line) {
    const char *dollar = strchr(_line, '$');
    if (dollar != NULL)
      _line = dollar;

    if (!VerifyNMEAChecksum(_line))
      return;

    NMEAInputLine line(_line);
    char cmd[32];
    line.Read(cmd, ARRAY_SIZE(cmd));

    if (strcmp(cmd, "$PFLAC") == 0)
      PFLAC(line);
    else if (strcmp(cmd, "$PFLAX") == 0)
      PFLAX();
  }
};

#endif
