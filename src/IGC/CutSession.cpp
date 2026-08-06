// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CutSession.hpp"
#include "IGCParser.hpp"
#include "io/FileLineReader.hpp"
#include "system/FileUtil.hpp"
#include "time/BrokenDate.hpp"
#include "time/BrokenTime.hpp"

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

namespace {

/**
 * How many of the most recently modified files to look inside.
 *
 * A Cut Session is by construction the most recently written file in the
 * directory: the crash happened minutes ago and nothing has touched the
 * directory since.  Examining a handful of the newest files is therefore
 * generous, and it bounds the cost of the scan -- which runs at takeoff, on
 * devices where reading every IGC file of a season would be felt.
 *
 * Ordering is by modification time, which is system-clock based and so may
 * be absolutely wrong after a hard reboot.  Only the *relative* order is
 * used here, which survives that; the staleness bound below is decided on
 * GPS time alone.
 */
constexpr std::size_t MAX_FILES_EXAMINED = 8;

struct IgcSummary {
  BrokenDate date = BrokenDate::Invalid();
  BrokenTime last_fix = BrokenTime::Invalid();
  unsigned b_record_count = 0;
  bool has_grecord = false;

  /**
   * Did this file come from a real GPS?
   *
   * IGCWriter::WriteHeader() emits the HFFXA fix-accuracy record only when
   * the Session was not simulated, so its absence marks a simulator file --
   * or a file XCSoar did not write, which is equally not ours to continue.
   */
  bool real = false;
};

/**
 * Extract the time from a B-record with a valid GPS fix.
 *
 * Deliberately a local copy of the equivalent in IgcMetaCache rather than a
 * shared utility: that one lives behind a UI-facing cache built on
 * coroutines and the asio thread, none of which belongs in the logger's
 * startup path or in a unit test.
 */
bool
ParseValidBRecordTime(const char *line, BrokenTime &time) noexcept
{
  /* time is at offset 1..6, fix validity at offset 24 */
  if (std::strlen(line) < 25)
    return false;

  if (!IGCParseTime(line + 1, time))
    return false;

  return line[24] == 'A';
}

/**
 * Read a whole IGC file, collecting everything the scan needs from it.
 *
 * One pass, because the three facts wanted live in different places: the
 * date in the header, the G-record at the end, and the fix count throughout.
 */
std::optional<IgcSummary>
SummariseIgcFile(Path path) noexcept
try {
  IgcSummary summary;

  FileLineReaderA reader{path};

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    switch (line[0]) {
    case 'B':
      if (BrokenTime time; ParseValidBRecordTime(line, time)) {
        summary.last_fix = time;
        ++summary.b_record_count;
      }
      break;

    case 'G':
      summary.has_grecord = true;
      break;

    case 'H':
      if (!summary.date.IsPlausible())
        IGCParseDateRecord(line, summary.date);

      if (std::string_view{line}.starts_with("HFFXA"))
        summary.real = true;
      break;
    }
  }

  return summary;
} catch (...) {
  /* an unreadable or malformed file is simply not a candidate */
  return std::nullopt;
}

using TimestampedPath =
  std::pair<std::chrono::system_clock::time_point, AllocatedPath>;

class IgcFileCollector final : public File::Visitor {
  std::vector<TimestampedPath> &files;

public:
  explicit IgcFileCollector(std::vector<TimestampedPath> &_files) noexcept
    :files(_files) {}

  void Visit(Path path, [[maybe_unused]] Path filename) override {
    files.emplace_back(File::GetLastModification(path), AllocatedPath{path});
  }
};

} // anonymous namespace

std::optional<CutSessionCandidate>
FindCutSession(Path directory, BrokenDateTime reference_utc,
               std::chrono::system_clock::duration max_age) noexcept
try {
  if (!reference_utc.IsPlausible())
    /* without a trustworthy GPS date and time there is nothing to compare
       against, and the system clock is not an acceptable substitute */
    return std::nullopt;

  std::vector<TimestampedPath> files;
  IgcFileCollector collector{files};
  Directory::VisitSpecificFiles(directory, "*.igc", collector);

  std::sort(files.begin(), files.end(),
            [](const TimestampedPath &a, const TimestampedPath &b){
              return a.first > b.first;
            });

  if (files.size() > MAX_FILES_EXAMINED)
    files.resize(MAX_FILES_EXAMINED);

  std::optional<CutSessionCandidate> best;

  for (auto &[modified, path] : files) {
    const auto summary = SummariseIgcFile(path);
    if (!summary)
      continue;

    if (summary->has_grecord)
      /* a Closed Session: XCSoar signed it, so it ended deliberately */
      continue;

    if (!summary->real)
      /* A simulated Session, which is never signed and so would otherwise
         qualify for ever.  Appending real fixes to it would put a genuine
         Flight inside a simulator file. */
      continue;

    if (summary->b_record_count == 0 || !summary->date.IsPlausible() ||
        !summary->last_fix.IsPlausible())
      continue;

    const BrokenDateTime last_fix_utc{summary->date, summary->last_fix};
    const auto age = reference_utc - last_fix_utc;
    if (age < std::chrono::system_clock::duration::zero() || age > max_age)
      /* stale, or dated after the current fix, which means one of the two
         clocks cannot be believed */
      continue;

    if (!best || last_fix_utc > best->last_fix_utc)
      best.emplace(CutSessionCandidate{std::move(path), last_fix_utc,
                                       summary->b_record_count});
  }

  return best;
} catch (...) {
  return std::nullopt;
}
