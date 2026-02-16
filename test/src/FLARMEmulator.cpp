// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARMEmulator.hpp"
#include "Device/Driver/FLARM/BinaryProtocol.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "util/Macros.hpp"
#include "util/SpanCast.hxx"
#include "util/StaticString.hxx"

#include <stdio.h>
#include <string.h>

using std::string_view_literals::operator""sv;

inline void
FLARMEmulator::PFLAC_S(NMEAInputLine &line) noexcept
{
  const auto name = line.ReadView();

  const auto value = line.Rest();
  StaticString<256> value_buffer;
  value_buffer.SetASCII(value);

  settings[std::string{name}] = value_buffer;

  char buffer[512];
  snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%.*s,%s",
           (int)name.size(), name.data(),
           value_buffer.c_str());
  PortWriteNMEA(*port, buffer, *env);
}

inline void
FLARMEmulator::PFLAC_R(NMEAInputLine &line) noexcept
{
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

inline void
FLARMEmulator::PFLAC(NMEAInputLine &line) noexcept
{
  const auto command = line.ReadView();
  if (command == "S"sv)
    PFLAC_S(line);
  else if (command == "R"sv)
    PFLAC_R(line);
}

inline void
FLARMEmulator::PFLAX() noexcept
{
  binary = true;
  binary_buffer.Clear();
}

inline size_t
FLARMEmulator::Unescape(const std::byte *const data, const std::byte *const end,
                        void *_dest, size_t length) noexcept
{
  std::byte *dest = (std::byte *)_dest;

  const std::byte *p;
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

inline void
FLARMEmulator::SendACK(uint16_t sequence_number) noexcept
{
  uint16_t payload = ToLE16(sequence_number);
  FLARM::FrameHeader header =
    FLARM::PrepareFrameHeader(sequence_number, FLARM::MessageType::ACK,
                              ReferenceAsBytes(payload));
  port->Write(FLARM::START_FRAME);
  FLARM::SendEscaped(*port, ReferenceAsBytes(header), *env,
                     std::chrono::seconds(2));
  FLARM::SendEscaped(*port, ReferenceAsBytes(payload), *env,
                     std::chrono::seconds(2));
}

inline size_t
FLARMEmulator::HandleBinary(const void *_data, size_t length) noexcept
{
  const std::byte *const data = (const std::byte *)_data, *end = data + length;
  const std::byte *p = data;

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
  case FLARM::MessageType::PING:
  case FLARM::MessageType::SELECTRECORD:
    SendACK(header.sequence_number);
    break;

  case FLARM::MessageType::EXIT:
    SendACK(header.sequence_number);
    binary = false;
    break;

  default:
    break;
  }

  return p - data;
}

inline void
FLARMEmulator::BinaryReceived(std::span<const std::byte> s) noexcept
{
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

bool
FLARMEmulator::DataReceived(std::span<const std::byte> s) noexcept
{
  if (binary) {
    BinaryReceived(s);
    return true;
  } else {
    fwrite(s.data(), 1, s.size(), stdout);
    return PortLineSplitter::DataReceived(s);
  }
}

bool
FLARMEmulator::LineReceived(const char *_line) noexcept
{
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
