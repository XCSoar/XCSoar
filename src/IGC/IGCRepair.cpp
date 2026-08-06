// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGCRepair.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileReader.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/CharUtil.hxx"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace {

/**
 * A single IGC record is well under a hundred bytes, so a window this size
 * cannot miss the final newline of a file XCSoar wrote.
 */
constexpr std::size_t TAIL_WINDOW = 4096;

/**
 * Could XCSoar have written this line?
 *
 * Deliberately permissive about *which* record types are acceptable.  The
 * failure this exists to catch is what a hard reboot actually leaves behind
 * -- a run of NUL bytes or other binary garbage where a filesystem block was
 * never written -- not an unexpected but well-formed record.  Rejecting a
 * file costs the pilot their Resume, so the test errs towards accepting.
 */
bool
IsPlausibleIgcLine(std::string_view line) noexcept
{
  if (line.empty() || !IsUpperAlphaASCII(line.front()))
    return false;

  return std::all_of(line.begin(), line.end(), IsPrintableASCII);
}

/**
 * Read the last bytes of a file into \a buffer.
 *
 * @return the bytes read, ending at the end of the file
 */
std::span<const std::byte>
ReadTail(Path path, std::span<std::byte> buffer)
{
  FileReader file{path};

  const auto size = file.GetSize();
  if (size == 0)
    throw std::runtime_error{"IGC file is empty"};

  const std::size_t window = std::min<uint_least64_t>(size, buffer.size());
  file.Seek((off_t)(size - window));

  std::size_t filled = 0;
  while (filled < window) {
    const std::size_t nbytes = file.Read(buffer.subspan(filled,
                                                        window - filled));
    if (nbytes == 0)
      throw std::runtime_error{"short read at end of IGC file"};

    filled += nbytes;
  }

  return buffer.first(window);
}

/**
 * Discard a final record that was cut off mid-write.
 */
void
TruncateTornFinalRecord(Path path)
{
  std::array<std::byte, TAIL_WINDOW> buffer;
  const auto tail = ReadTail(path, buffer);

  if (tail.back() == std::byte{'\n'})
    /* the last record is complete; nothing to repair */
    return;

  const auto last_newline = std::find(tail.rbegin(), tail.rend(),
                                      std::byte{'\n'});
  if (last_newline == tail.rend())
    throw std::runtime_error{"IGC file ends in an unterminated record"};

  /* rbegin() is the final byte, so the distance to the newline gives the
     number of bytes to drop */
  const auto torn_bytes = std::distance(tail.rbegin(), last_newline);
  const uint64_t keep = File::GetSize(path) - (uint64_t)torn_bytes;

  if (!File::Truncate(path, keep))
    throw std::runtime_error{"failed to truncate torn IGC record"};
}

/**
 * Reject a file with damage anywhere but at its very end, which truncation
 * cannot repair.
 */
void
ValidateEveryLine(Path path)
{
  FileLineReaderA reader{path};

  bool any = false;

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (!IsPlausibleIgcLine(line))
      throw std::runtime_error{"IGC file is corrupt"};

    any = true;
  }

  if (!any)
    throw std::runtime_error{"IGC file has no records"};
}

} // anonymous namespace

void
RepairIgcForAppend(Path path)
{
  TruncateTornFinalRecord(path);
  ValidateEveryLine(path);
}
