// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmRangeComputer.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "LogFile.hpp"

void
FlarmRangeComputer::Load(Path _path) noexcept
{
  path = _path;

  if (!File::Exists(path))
    return;

  try {
    FileLineReaderA reader(path);
    FlarmRangeEstimate loaded;
    LoadFlarmRangeEstimate(loaded, reader);

    const std::lock_guard lock{mutex};
    estimate = loaded;
    modified = false;
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to load the FLARM range estimate");
  }
}

void
FlarmRangeComputer::Save() noexcept
{
  if (path == nullptr)
    return;

  /* under the lock, so that the calculation thread and a reset from
     the user interface never write the file at the same time; it is
     only a few kilobytes */
  const std::lock_guard lock{mutex};
  if (!modified)
    return;

  try {
    FileOutputStream file(path);
    BufferedOutputStream os(file);
    SaveFlarmRangeEstimate(estimate, os);
    os.Flush();
    file.Commit();
    modified = false;
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to save the FLARM range estimate");
  }
}

void
FlarmRangeComputer::Process(const NMEAInfo &basic, bool flying) noexcept
{
  const bool landed = was_flying && !flying;

  {
    const std::lock_guard lock{mutex};
    const auto count = estimate.GetCount();
    estimator.Process(estimate, basic, flying);
    if (landed)
      /* count the contacts still open before saving */
      estimator.Flush(estimate);
    if (estimate.GetCount() != count)
      modified = true;
  }

  if (landed)
    Save();
  else if (flying && save_clock.CheckUpdate(std::chrono::minutes(10)))
    Save();

  was_flying = flying;
}

void
FlarmRangeComputer::Close() noexcept
{
  {
    const std::lock_guard lock{mutex};
    const auto count = estimate.GetCount();
    estimator.Flush(estimate);
    if (estimate.GetCount() != count)
      modified = true;
  }

  Save();
}

void
FlarmRangeComputer::Reset() noexcept
{
  {
    const std::lock_guard lock{mutex};
    estimate.Clear();
    /* drop the open contacts too, they began before the reset */
    estimator = {};
    modified = true;
  }

  Save();
}
