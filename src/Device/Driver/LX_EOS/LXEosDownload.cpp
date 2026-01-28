// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "LXEosDevice.hpp"
#include "Operation/Operation.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileOutputStream.hxx"
#include "system/Path.hpp"
#include "util/AllocatedArray.hxx"
#include "util/ByteOrder.hxx"
#include "util/CRC8.hpp"
#include "util/ScopeExit.hxx"

bool
LXEosDevice::ReadFlightList(RecordedFlightList& flight_list,
                            OperationEnvironment& env)
{
  port.StopRxThread();
  env.SetProgressRange(1);
  env.SetProgressPosition(0);

  try {
    uint8_t flight_count = GetNumberOfFlights(env);

    // Maximum number of flights to be obtained is limited by length of the list
    if (flight_count > flight_list.max_size())
      flight_count = flight_list.max_size();

    env.SetProgressRange(flight_count + 1);
    env.SetProgressPosition(1);
    for (int i = 0; i < flight_count; i++) {
      RecordedFlightInfo flight;

      // Try up to 5 times before giving up
      constexpr int max_attempt = 5;
      for (int attempt = 1; attempt <= max_attempt; attempt++) {
        try {
          flight = GetFlightInfo(i + 1, env);
          break;
        } catch (...) {
          if (attempt == max_attempt)
            throw;
        }
      }

      flight_list.append(flight);
      env.SetProgressPosition(i + 2);
    }
    port.StartRxThread();
    return true;
  } catch (...) {
    port.StartRxThread();
    return false;
  }
}

bool
LXEosDevice::DownloadFlight(const RecordedFlightInfo& flight,
                            Path path,
                            OperationEnvironment& env,
                            [[maybe_unused]] unsigned *resume_row)
{
  FileOutputStream fos(path);
  BufferedOutputStream bos(fos);

  port.StopRxThread();

  uint16_t flight_id = flight.internal.lx_eos.flight_id;
  uint32_t bytes_remaining = flight.internal.lx_eos.file_size;

  env.SetProgressRange(100);
  env.SetProgressPosition(0);

  try {
    // Download blocks until remaining bytes reach zero
    for (uint16_t block_id = 0; bytes_remaining > 0; block_id++) {
      AllocatedArray<std::byte> block;

      // Try up to 5 times before giving up
      constexpr int max_attempt = 5;
      for (int attempt = 1; attempt <= max_attempt; attempt++) {
        try {
          block = GetFlightLogBlock(flight_id, block_id, env);
          break;
        } catch (...) {
          if (attempt == max_attempt)
            throw;
        }
      }

      bos.Write(block);

      // Make sure that bytes remaining cannot underflow
      if (block.size() > bytes_remaining)
        throw std::runtime_error("Oversize read");

      bytes_remaining -= block.size();

      // Progress in percents downloaded
      float progress =
        100.0f *
        (1.0f - static_cast<float>(bytes_remaining) /
                  static_cast<float>(flight.internal.lx_eos.file_size));
      env.SetProgressPosition(static_cast<unsigned int>(floor(progress)));
    }

    port.Flush();

    fos.Commit();

    env.SetProgressPosition(100);

    port.StartRxThread();
    return true;
  } catch (...) {
    port.StartRxThread();
    return false;
  }
}

uint8_t
LXEosDevice::GetNumberOfFlights(OperationEnvironment& env)
{
  // Request
  EosGetNumOfFlights data;
  WriteAndWaitForACK(std::span{reinterpret_cast<std::byte*>(&data),sizeof(data)}, env);

  // Read response (ACK byte already read)
  EosNumOfFlightsResponse response;
  port.FullRead(
    { reinterpret_cast<std::byte*>(&response) + 1, sizeof(response) - 1 },
    env,
    communication_timeout);

  // Check CRC
  if (UpdateCRC8(std::span{reinterpret_cast<std::byte*>(&data),sizeof(data)},
                 std::byte{ 0xFF }) != std::byte{ 0x00 })
    throw std::runtime_error("Bad CRC");

  return response.number_of_flights;
}

RecordedFlightInfo
LXEosDevice::GetFlightInfo(const uint8_t index, OperationEnvironment& env)
{
  if (index == 0) // Invalid
    throw std::runtime_error("Invalid index");

  // Request
  EosRequestFlightInfo data;
  data.flight_id = static_cast<uint16_t>(index);
  data.crc = UpdateCRC8(std::span{reinterpret_cast<std::byte*>(&data),sizeof(data)-1},
                        std::byte{ 0xFF });
  WriteAndWaitForACK(std::span{reinterpret_cast<std::byte*>(&data),sizeof(data)}, env);

  // Read response (ACK byte already read)
  EosFlightInfoResponse info;
  port.FullRead(
    std::span{ reinterpret_cast<std::byte*>(&info) + 1, sizeof(info) - 1 },
    env,
    communication_timeout);

  if (UpdateCRC8(std::span{reinterpret_cast<std::byte*>(&info),sizeof(info)},
                 std::byte{ 0xFF }) != std::byte{ 0x00 }) // Check CRC
    throw std::runtime_error("Bad CRC");

  /* Flight ID for downloading is the index (1 = newest),
  not the ID from this message */
  RecordedFlightInfo flight;
  flight.internal.lx_eos.flight_id = index;
  flight.date = BrokenDate::FromJulianDate(FromLE32(info.uiDate));
  flight.start_time = BrokenTime::FromSecondOfDay(FromLE32(info.uiTakeOff));
  flight.end_time = BrokenTime::FromSecondOfDay(FromLE32(info.uiLanding));
  flight.internal.lx_eos.file_size = FromLE32(info.size);
  return flight;
}

AllocatedArray<std::byte>
LXEosDevice::GetFlightLogBlock(uint16_t flight_id,
                               uint16_t block_id,
                               OperationEnvironment& env)
{
  // Request
  EosRequestFlightBlock data;
  data.flight_id = flight_id;
  data.block_id = block_id;
  data.crc = UpdateCRC8(std::span{reinterpret_cast<std::byte*>(&data),sizeof(data)-1},
                        std::byte{ 0xFF });
  WriteAndWaitForACK(std::span{reinterpret_cast<std::byte*>(&data),sizeof(data)}, env);

  // The ACK byte was already read, read remaining bytes of header
  EosFlightBlockResponseHeader header;
  port.FullRead(
    std::span{ reinterpret_cast<std::byte*>(&header) + 1, sizeof(header) - 1 },
    env,
    communication_timeout);

  // Check that received ID matches the requested one
  if (header.block_id != block_id)
    throw std::runtime_error("Bad ID");

  AllocatedArray<std::byte> block;
  block.ResizeDiscard(FromLE16(header.block_size));

  port.FullRead({ block.data(), block.size() }, env, communication_timeout);

  std::byte crc_byte;
  port.WaitAndRead(std::span{ &crc_byte, 1 }, env, communication_timeout);

  /* Message has 3 parts: header, block, and CRC byte.
  Calculate CRC from all 3 parts, result should be 0x00 */
  std::byte crc = std::byte{ 0xFF };
  crc = UpdateCRC8(std::span{reinterpret_cast<std::byte*>(&header),sizeof(header)}, crc);
  crc =
    UpdateCRC8(std::as_bytes(std::span{ block.data(), block.size() }), crc);
  if (UpdateCRC8(std::span{ &crc_byte, sizeof(crc_byte) }, crc) !=
      std::byte{ 0x00 })
    throw std::runtime_error("Bad CRC");

  return block;
}
