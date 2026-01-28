// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/RecordedFlight.hpp"
#include "Operation/Operation.hpp"
#include "util/ByteOrder.hxx"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"

#include <memory>

#include <stdlib.h>

static void
Copy(BrokenDate &dest, const CAI302::DateTime &src)
{
  dest.year = src.year + 2000; /* Y2100 problem! */
  dest.month = src.month;
  dest.day = src.day;
}

static void
Copy(BrokenTime &dest, const CAI302::DateTime &src)
{
  dest.hour = src.hour;
  dest.minute = src.minute;
  dest.second = src.second;
}

static void
Copy(RecordedFlightInfo &dest, unsigned index,
     const CAI302::FileList::FileInfo &src)
{
  Copy(dest.date, src.start_utc);
  Copy(dest.start_time, src.start_utc);
  Copy(dest.end_time, src.end_utc);

  dest.internal.cai302 = index;
}

static bool
ReadFlightListInner(Port &port, RecordedFlightList &flight_list,
                    OperationEnvironment &env)
{
  env.SetProgressRange(8);

  for (unsigned i = 0; i < 8 && !flight_list.full(); ++i) {
    CAI302::FileList file_list;
    if (!CAI302::UploadFileList(port, i, file_list, env))
      break;

    for (unsigned j = 0; j < 8 && !flight_list.full(); ++j) {
      const CAI302::FileList::FileInfo &file = file_list.files[j];
      if (file.start_utc.month > 0)
        Copy(flight_list.append(), i * 8 + j, file);
    }

    env.SetProgressPosition(i);
  }

  return !flight_list.empty();
}

bool
CAI302Device::ReadFlightList(RecordedFlightList &flight_list,
                             OperationEnvironment &env)
{
  if (!EnableBulkMode(env))
    return false;

  try {
    UploadMode(env);
    bool success = ReadFlightListInner(port, flight_list, env);
    DisableBulkMode(env);
    return success;
  } catch (...) {
    mode = Mode::UNKNOWN;

    try {
      DisableBulkMode(env);
    } catch (...) {
    }

    throw;
  }
}

static bool
DownloadFlightInner(Port &port, const RecordedFlightInfo &flight,
                    Path path, OperationEnvironment &env)
{
  assert(flight.internal.cai302 < 64);

  FileOutputStream fos(path);
  BufferedOutputStream os(fos);

  CAI302::FileASCII file_ascii;
  if (!UploadFileASCII(port, flight.internal.cai302, file_ascii, env))
    return false;

  unsigned bytes_per_block = file_ascii.bytes_per_block;
  unsigned num_blocks = file_ascii.num_blocks;
  env.SetProgressRange(num_blocks);

  unsigned allocated_size = sizeof(CAI302::FileData) + bytes_per_block;
  std::unique_ptr<uint8_t[]> allocated(new uint8_t[allocated_size]);
  // TODO: alignment?
  CAI302::FileData *header = (CAI302::FileData *)(void *)allocated.get();
  const std::byte *data = reinterpret_cast<const std::byte *>(header + 1);

  unsigned current_block = 0;
  unsigned valid_bytes;
  do {
    int i = CAI302::UploadFileData(port, true, {(std::byte *)header, allocated_size}, env);
    if (i < (int)sizeof(*header))
      return false;

    i -= sizeof(*header);

    valid_bytes = FromBE16(header->valid_bytes);
    if ((unsigned)i < valid_bytes)
      return false;

    os.Write({data, valid_bytes});

    env.SetProgressPosition(current_block++);
  } while (valid_bytes == bytes_per_block);

  allocated.reset();

  CAI302::FileSignatureASCII signature;
  if (!CAI302::UploadFileSignatureASCII(port, signature, env))
    return false;

  valid_bytes = FromBE16(signature.size);
  if (valid_bytes > sizeof(signature.signature))
    return false;

  os.Write(std::as_bytes(std::span{signature.signature, valid_bytes}));

  os.Flush();
  fos.Commit();

  return true;
}

bool
CAI302Device::DownloadFlight(const RecordedFlightInfo &flight,
                             Path path,
                             OperationEnvironment &env,
                             [[maybe_unused]] unsigned *resume_row)
{
  assert(flight.internal.cai302 < 64);

  if (!EnableBulkMode(env))
    return false;

  try {
    UploadMode(env);
    bool success = DownloadFlightInner(port, flight, path, env);
    DisableBulkMode(env);
    return success;
  } catch (...) {
    mode = Mode::UNKNOWN;

    try {
      DisableBulkMode(env);
    } catch (...) {
    }

    throw;
  }
}
