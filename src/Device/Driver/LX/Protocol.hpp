// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Port.hpp"
#include "util/Compiler.h"

#include <cstddef>
#include <cstdint>
#include <utility>

class OperationEnvironment;

namespace LX {

static constexpr unsigned NUMTPS = 12;

enum Command {
  PREFIX = 0x02,
  ACK = 0x06,
  SYN = 0x16,
  WRITE_FLIGHT_INFO = 0xCA,
  READ_MEMORY_SECTION = 0xcc,
  READ_FLIGHT_LIST = 0xcd,
  SEEK_MEMORY = 0xce,
  WRITE_CONTEST_CLASS = 0xD0,
  READ_LOGGER_DATA = 0xe6,
};

#pragma pack(push, 1) // force byte alignment

/**
 * Strings have extra byte for NULL.
 */
struct Pilot {
  uint8_t unknown1[3];
  char PilotName[19];
  char GliderType[12];
  char GliderID[8];
  char CompetitionID[4];
  uint8_t unknown2[73];
} gcc_packed;

/**
 * Strings have extra byte for NULL.
 */
struct Declaration {
  uint8_t unknown1[5];
  uint8_t dayinput;
  uint8_t monthinput;
  uint8_t yearinput;
  uint8_t dayuser;
  uint8_t monthuser;
  uint8_t yearuser;
  int16_t taskid;
  uint8_t numtps;
  uint8_t tptypes[NUMTPS];
  int32_t Longitudes[NUMTPS];
  int32_t Latitudes[NUMTPS];
  char WaypointNames[NUMTPS][9];
} gcc_packed;

struct MemoryAddress32 {
  uint8_t address1;
  uint8_t address0;
  uint8_t address3;
  uint8_t address2;
} gcc_packed;

struct MemoryAddress24 {
  uint8_t address[3];

  MemoryAddress24 &operator=(const MemoryAddress32 &other) {
    address[0] = other.address0;
    address[1] = other.address1;
    address[2] = other.address2;
    return *this;
  }

  MemoryAddress24 &operator=(const uint8_t other[3]) {
    address[0] = other[0];
    address[1] = other[1];
    address[2] = other[2];
    return *this;
  }
} gcc_packed;

struct FlightInfo {
  uint8_t valid;
  MemoryAddress32 start_address;
  MemoryAddress32 end_address;
  char date[9];
  char start_time[9];
  char stop_time[9];
  uint8_t dummy0[4];
  char pilot[52];
  uint16_t logger_id;
  uint8_t flight_no; /* ? */

  bool IsValid() const {
    return valid == 1 && date[8] == 0 && start_time[8] == 0 &&
      stop_time[8] == 0;
  }
} gcc_packed;

struct SeekMemory {
  MemoryAddress24 start_address, end_address;
} gcc_packed;

struct MemorySection {
  static constexpr unsigned N = 0x10;

  uint16_t lengths[N];
} gcc_packed;

/**
 * Strings have extra byte for NULL.
 */
struct ContestClass {
  char contest_class[9];
} gcc_packed;

#pragma pack(pop)

static inline void
SendSYN(Port &port)
{
  port.Write(SYN);
}

static inline void
ExpectACK(Port &port, OperationEnvironment &env,
          std::chrono::steady_clock::duration timeout=std::chrono::seconds(2))
{
  port.WaitForChar(ACK, env, timeout);
}

/**
 * Send SYN and wait for ACK.
 *
 * @return true on success
 */
static inline void
Connect(Port &port, OperationEnvironment &env,
        std::chrono::steady_clock::duration timeout=std::chrono::milliseconds(500))
{
  SendSYN(port);
  ExpectACK(port, env, timeout);
}

/**
 * Enter command mode: flush all buffers, configure a sensible
 * receive timeout, sends SYN three times and waits for ACK.
 */
void CommandMode(Port &port, OperationEnvironment &env);

/**
 * Enter command mode without waiting for ACK.
 */
void
CommandModeQuick(Port &port, OperationEnvironment &env);

static inline void
SendCommand(Port &port, Command command)
{
  port.Write(PREFIX);
  port.Write(command);
}

void
SendPacket(Port &port, Command command,
           std::span<const std::byte> payload,
           OperationEnvironment &env,
           std::chrono::steady_clock::duration timeout=std::chrono::seconds(5));

bool
ReceivePacket(Port &port, Command command,
              std::span<std::byte> dest, OperationEnvironment &env,
              std::chrono::steady_clock::duration first_timeout,
              std::chrono::steady_clock::duration subsequent_timeout,
              std::chrono::steady_clock::duration total_timeout);

/**
 * Wrapper for ReceivePacket() which can retry on failure.  Before
 * each retry, it performs a full handshake with the device to reset
 * its command parser.
 */
bool
ReceivePacketRetry(Port &port, Command command,
                   std::span<std::byte> dest, OperationEnvironment &env,
                   std::chrono::steady_clock::duration first_timeout,
                   std::chrono::steady_clock::duration subsequent_timeout,
                   std::chrono::steady_clock::duration total_timeout,
                   unsigned n_retries);

[[gnu::const]]
std::byte
calc_crc_char(std::byte d, std::byte crc) noexcept;

[[gnu::pure]]
std::byte
calc_crc(std::span<const std::byte> src, std::byte crc) noexcept;

bool
ReadCRC(Port &port, std::span<std::byte> dest, OperationEnvironment &env,
        std::chrono::steady_clock::duration first_timeout,
        std::chrono::steady_clock::duration subsequent_timeout,
        std::chrono::steady_clock::duration total_timeout);

class CRCWriter {
  Port &port;
  std::byte crc{0xff};

public:
  explicit constexpr CRCWriter(Port &_port) noexcept:port(_port) {}

  bool Write(std::span<const std::byte> src,
             OperationEnvironment &env,
             std::chrono::steady_clock::duration timeout=std::chrono::seconds(5)) {
    port.FullWrite(src, env, timeout);

    crc = calc_crc(src, crc);
    return true;
  }

  void Write(std::byte value) {
    port.Write(value);
    crc = calc_crc_char(value, crc);
  }

  /**
   * Write the CRC, and reset it, so the object can be reused.
   */
  void Flush() {
    port.Write(std::exchange(crc, std::byte{0xff}));
  }
};

} // namespace LX
