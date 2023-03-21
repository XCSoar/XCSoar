// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DeviceEmulator.hpp"
#include "Device/Util/LineSplitter.hpp"
#include "Device/Driver/FLARM/BinaryProtocol.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "util/Macros.hpp"
#include "util/StaticFifoBuffer.hxx"
#include "util/StaticString.hxx"

#include <string>
#include <map>
#include <stdio.h>
#include <string.h>

using std::string_view_literals::operator""sv;

class FLARMEmulator : public Emulator, PortLineSplitter {
  std::map<std::string, std::string, std::less<>> settings;

  bool binary;
  StaticFifoBuffer<std::byte, 256u> binary_buffer;

public:
  FLARMEmulator():binary(false) {
    handler = this;
  }

private:
  void PFLAC_S(NMEAInputLine &line) {
    const auto name = line.ReadView();

    const auto value = line.Rest();
    NarrowString<256> value_buffer;
    value_buffer.SetASCII(value);

    settings[std::string{name}] = value_buffer;

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%.*s,%s",
             (int)name.size(), name.data(),
             value_buffer.c_str());
    PortWriteNMEA(*port, buffer, *env);
  }

  void PFLAC_R(NMEAInputLine &line) {
    const auto name = line.ReadView();

    auto i = settings.find(name);
    if (i == settings.end())
      return;

    const char *value = i->second.c_str();

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%.*s,%s",
             (int)name.size(), name.data(), value);
    PortWriteNMEA(*port, buffer, *env);
  }

  void PFLAC(NMEAInputLine &line) {
    const auto command = line.ReadView();
    if (command == "S"sv)
      PFLAC_S(line);
    else if (command == "R"sv)
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

  void SendACK(uint16_t sequence_number) {
    uint16_t payload = ToLE16(sequence_number);
    FLARM::FrameHeader header =
      FLARM::PrepareFrameHeader(sequence_number, FLARM::MT_ACK,
                                &payload, sizeof(payload));
    port->Write(FLARM::START_FRAME);
    FLARM::SendEscaped(*port, &header, sizeof(header), *env,
                       std::chrono::seconds(2));
    FLARM::SendEscaped(*port, &payload, sizeof(payload), *env,
                       std::chrono::seconds(2));
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

  void BinaryReceived(std::span<const std::byte> s) noexcept {
    const auto *data = s.data(), *end = data + s.size();

    do {
      /* append new data to buffer, as much as fits there */
      auto range = binary_buffer.Write();
      if (range.empty()) {
        /* overflow: reset buffer to recover quickly */
        binary_buffer.Clear();
        continue;
      }

      size_t nbytes = std::min(range.size(), size_t(end - data));
      std::copy_n(data, nbytes, range.begin());
      data += nbytes;
      binary_buffer.Append(nbytes);

      while (true) {
        range = binary_buffer.Read();
        if (range.empty())
          break;

        size_t nbytes = HandleBinary(range.data(), range.size());
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
  bool DataReceived(std::span<const std::byte> s) noexcept override {
    if (binary) {
      BinaryReceived(s);
      return true;
    } else {
      fwrite(s.data(), 1, s.size(), stdout);
      return PortLineSplitter::DataReceived(s);
    }
  }

  bool LineReceived(const char *_line) noexcept override {
    const char *dollar = strchr(_line, '$');
    if (dollar != NULL)
      _line = dollar;

    if (!VerifyNMEAChecksum(_line))
      return true;

    NMEAInputLine line(_line);

    const auto cmd = line.ReadView();
    if (cmd == "$PFLAC"sv)
      PFLAC(line);
    else if (cmd == "$PFLAX"sv)
      PFLAX();

    return true;
  }
};
