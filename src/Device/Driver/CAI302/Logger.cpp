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

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/RecordedFlight.hpp"
#include "Operation/Operation.hpp"
#include "OS/ByteOrder.hpp"
#include "OS/Path.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"

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

  return !flight_list.empty() && !env.IsCancelled();
}

bool
CAI302Device::ReadFlightList(RecordedFlightList &flight_list,
                             OperationEnvironment &env)
{
  if (!EnableBulkMode(env))
    return false;

  if (!UploadMode(env)) {
    DisableBulkMode(env);
    return false;
  }

  if (!ReadFlightListInner(port, flight_list, env)) {
    mode = Mode::UNKNOWN;
    DisableBulkMode(env);
    return false;
  }

  DisableBulkMode(env);
  return true;
}

static bool
DownloadFlightInner(Port &port, const RecordedFlightInfo &flight,
                    Path path, OperationEnvironment &env)
{
  assert(flight.internal.cai302 < 64);

  FileOutputStream fos(path);
  BufferedOutputStream os(fos);

  CAI302::FileASCII file_ascii;
  if (!UploadFileASCII(port, flight.internal.cai302, file_ascii, env) ||
      env.IsCancelled())
    return false;

  unsigned bytes_per_block = file_ascii.bytes_per_block;
  unsigned num_blocks = file_ascii.num_blocks;
  env.SetProgressRange(num_blocks);

  unsigned allocated_size = sizeof(CAI302::FileData) + bytes_per_block;
  std::unique_ptr<uint8_t> allocated(new uint8_t[allocated_size]);
  // TODO: alignment?
  CAI302::FileData *header = (CAI302::FileData *)(void *)allocated.get();
  void *data = header + 1;

  unsigned current_block = 0;
  unsigned valid_bytes;
  do {
    int i = UploadFileData(port, true, header, allocated_size, env);
    if (i < (int)sizeof(*header))
      return false;

    i -= sizeof(*header);

    valid_bytes = FromBE16(header->valid_bytes);
    if ((unsigned)i < valid_bytes)
      return false;

    os.Write(data, valid_bytes);

    env.SetProgressPosition(current_block++);
  } while (valid_bytes == bytes_per_block);

  allocated.reset();

  CAI302::FileSignatureASCII signature;
  if (!CAI302::UploadFileSignatureASCII(port, signature, env))
    return false;

  valid_bytes = FromBE16(signature.size);
  if (valid_bytes > sizeof(signature.signature))
    return false;

  os.Write(signature.signature, valid_bytes);

  os.Flush();
  fos.Commit();

  return true;
}

bool
CAI302Device::DownloadFlight(const RecordedFlightInfo &flight,
                             Path path,
                             OperationEnvironment &env)
{
  assert(flight.internal.cai302 < 64);

  if (!EnableBulkMode(env))
    return false;

  if (!UploadMode(env)) {
    DisableBulkMode(env);
    return false;
  }

  try {
    if (!DownloadFlightInner(port, flight, path, env)) {
      mode = Mode::UNKNOWN;
      DisableBulkMode(env);
      return false;
    }
  } catch (...) {
    mode = Mode::UNKNOWN;
    try {
      DisableBulkMode(env);
    } catch (...) {
    }
    throw;
  }

  DisableBulkMode(env);
  return true;
}
