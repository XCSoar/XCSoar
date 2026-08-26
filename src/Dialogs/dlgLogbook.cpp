// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Computer/IGCFlightTimes.hpp"
#include "IGC/IGCParser.hpp"
#include "Computer/Settings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "io/FileLineReader.hpp"
#include "system/FileUtil.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/BrokenTime.hpp"
#include "ui/event/Timer.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr unsigned LOGBOOK_PAGE_SIZE = 10;

struct LogbookEntry {
  AllocatedPath path;
  std::chrono::system_clock::time_point mtime{};

  bool parsed = false;
  bool valid = false;
  bool started_too_late = false;
  bool ended_too_early = false;

  BrokenDate date = BrokenDate::Invalid();
  BrokenTime start_local = BrokenTime::Invalid();
  BrokenTime end_local = BrokenTime::Invalid();
  std::chrono::system_clock::duration duration =
    std::chrono::seconds{-1};
};

[[gnu::pure]]
static double
TakeoffSpeedFromSettings() noexcept
{
  const auto &polar =
    CommonInterface::GetComputerSettings().polar.glide_polar_task;
  return polar.IsValid() ? polar.GetVTakeoff() : DEFAULT_IGC_TAKEOFF_SPEED;
}

/**
 * Duration between displayed HH:MM takeoff and landing (seconds of
 * the clock are ignored so 14:12→14:13 is always one minute).
 */
[[gnu::pure]]
static std::chrono::system_clock::duration
DurationFromDisplayedTimes(const BrokenTime &start,
                           const BrokenTime &end) noexcept
{
  if (!start.IsPlausible() || !end.IsPlausible())
    return std::chrono::seconds{-1};

  unsigned start_m = start.hour * 60u + start.minute;
  unsigned end_m = end.hour * 60u + end.minute;
  if (end_m < start_m)
    end_m += 24u * 60u;

  return std::chrono::minutes{end_m - start_m};
}

[[gnu::pure]]
static BrokenDate
DateFromFilename(Path path) noexcept
{
  const Path base = path.GetBase();
  if (base == nullptr)
    return BrokenDate::Invalid();

  const char *name = base.c_str();
  unsigned year, month, day;
  if (sscanf(name, "%04u-%02u-%02u", &year, &month, &day) != 3)
    return BrokenDate::Invalid();

  BrokenDate date(year, month, day);
  return date.IsPlausible() ? date : BrokenDate::Invalid();
}

static BrokenDate
DateFromHFDTE(Path path) noexcept
{
  try {
    FileLineReaderA reader(path);
    char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      BrokenDate date;
      if (IGCParseDateRecord(line, date))
        return date;
    }
  } catch (...) {
  }

  return BrokenDate::Invalid();
}

class LogbookWidget final : public ListWidget {
  TwoTextRowsRenderer row_renderer;
  std::vector<LogbookEntry> entries;
  unsigned shown = 0;

  /** Parse one pending row at a time so the dialog stays responsive. */
  UI::Timer parse_timer{[this]{ ParseNextPending(); }};

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc,
               row_renderer.CalculateLayout(*look.list.font_bold,
                                            look.small_font));
    ScanIgcFiles();
    shown = std::min(LOGBOOK_PAGE_SIZE, (unsigned)entries.size());
    RefreshLength();
    ScheduleParseNext();
  }

  void Unprepare() noexcept override {
    parse_timer.Cancel();
    DeleteWindow();
  }

protected:
  bool CanActivateItem(unsigned i) const noexcept override {
    return IsMoreRow(i);
  }

  void OnActivateItem(unsigned i) noexcept override {
    if (!IsMoreRow(i))
      return;

    shown = std::min(shown + LOGBOOK_PAGE_SIZE,
                     (unsigned)entries.size());
    RefreshLength();
    GetList().Invalidate();
    ScheduleParseNext();
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (entries.empty()) {
      row_renderer.DrawFirstRow(canvas, rc, _("No flights"));
      row_renderer.DrawSecondRow(canvas, rc,
                                 _("No IGC files found in the logs folder."));
      return;
    }

    if (IsMoreRow(idx)) {
      row_renderer.DrawFirstRow(canvas, rc, _("More…"));
      StaticString<64> detail;
      detail.UnsafeFormat("%u / %u",
                          shown, (unsigned)entries.size());
      row_renderer.DrawSecondRow(canvas, rc, detail.c_str());
      return;
    }

    if (idx >= shown)
      return;

    const auto &entry = entries[idx];

    StaticString<64> first_row;
    if (entry.date.IsPlausible())
      first_row.UnsafeFormat("%02u/%02u/%04u",
                             entry.date.day, entry.date.month,
                             entry.date.year);
    else
      first_row = "--/--/----";

    StaticString<160> second_row;
    if (!entry.parsed) {
      second_row = _("…");
    } else if (!entry.valid) {
      second_row = _("(no takeoff/landing)");
    } else {
      StaticString<16> start_text, end_text, duration_text;
      FormatTime(start_text, entry.start_local);
      FormatTime(end_text, entry.end_local);

      if (entry.duration.count() >= 0) {
        const BrokenTime broken =
          BrokenTime::FromSinceMidnight(entry.duration);
        duration_text.UnsafeFormat("%02u:%02u",
                                   broken.hour, broken.minute);
      } else
        duration_text = "--:--";

      second_row.UnsafeFormat("%s - %s  (%s)",
                              start_text.c_str(), end_text.c_str(),
                              duration_text.c_str());

      if (entry.started_too_late || entry.ended_too_early) {
        second_row.append("  ");
        second_row.append(_("warn:"));
        second_row.append(" ");
        if (entry.started_too_late) {
          second_row.append(_("started too late"));
          if (entry.ended_too_early)
            second_row.append(", ");
        }
        if (entry.ended_too_early)
          second_row.append(_("ended too early"));
      }
    }

    row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());
    row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
  }

private:
  [[gnu::pure]]
  bool HasMore() const noexcept {
    return shown < entries.size();
  }

  [[gnu::pure]]
  bool IsMoreRow(unsigned i) const noexcept {
    return HasMore() && i == shown;
  }

  void RefreshLength() noexcept {
    if (entries.empty())
      GetList().SetLength(1);
    else
      GetList().SetLength(shown + (HasMore() ? 1 : 0));
  }

  static void FormatTime(StaticString<16> &dest,
                         const BrokenTime &time) noexcept {
    if (time.IsPlausible())
      dest.UnsafeFormat("%02u:%02u", time.hour, time.minute);
    else
      dest = "--:--";
  }

  void ScheduleParseNext() noexcept {
    if (!parse_timer.IsPending())
      parse_timer.Schedule(std::chrono::milliseconds{1});
  }

  void ParseNextPending() noexcept {
    for (unsigned i = 0; i < shown; ++i) {
      auto &entry = entries[i];
      if (entry.parsed)
        continue;

      ParseIgc(entry);
      entry.parsed = true;
      GetList().Invalidate();
      ScheduleParseNext();
      return;
    }
  }

  void ScanIgcFiles() noexcept {
    entries.clear();

    try {
      const auto logs_path = LocalPath("logs");
      if (!Directory::Exists(logs_path))
        return;

      class Visitor final : public File::Visitor {
        std::vector<LogbookEntry> &list;

      public:
        explicit Visitor(std::vector<LogbookEntry> &_list) noexcept
          :list(_list) {}

        void Visit(Path path, Path filename) override {
          if (!filename.EndsWithIgnoreCase(".igc"))
            return;

          try {
            LogbookEntry entry;
            entry.path = path;
            entry.mtime = File::GetLastModification(path);
            entry.date = DateFromFilename(path);
            list.push_back(std::move(entry));
          } catch (...) {
            /* Skip this file: allocation / path failures must not
               escape VisitFiles (may run under a noexcept boundary). */
          }
        }
      } visitor(entries);

      Directory::VisitFiles(logs_path, visitor, true);

      /* Newest first by filename (YYYY-MM-DD-… IGC names sort
         lexicographically). */
      std::sort(entries.begin(), entries.end(),
                [](const LogbookEntry &a, const LogbookEntry &b) noexcept {
                  const Path a_base = a.path.GetBase();
                  const Path b_base = b.path.GetBase();
                  const char *a_name = a_base != nullptr ? a_base.c_str() : "";
                  const char *b_name = b_base != nullptr ? b_base.c_str() : "";
                  return strcmp(a_name, b_name) > 0;
                });
    } catch (...) {
      entries.clear();
    }
  }

  static void ParseIgc(LogbookEntry &entry) noexcept {
    if (!entry.date.IsPlausible()) {
      BrokenDate date = DateFromHFDTE(entry.path);
      if (!date.IsPlausible() &&
          entry.mtime != std::chrono::system_clock::time_point{})
        date = BrokenDateTime{entry.mtime};
      if (date.IsPlausible())
        entry.date = date;
    }

    IGCFlightTimesResult detected;
    if (!DetectIGCFlightTimes(entry.path, TakeoffSpeedFromSettings(),
                              detected))
      return;

    const auto utc_offset =
      CommonInterface::GetComputerSettings().utc_offset.ToDuration();

    const BrokenDateTime start_local_dt =
      detected.takeoff_utc + utc_offset;
    BrokenDateTime end_local_dt =
      detected.landing_utc + utc_offset;

    if (detected.landing_utc < detected.takeoff_utc)
      end_local_dt = end_local_dt + std::chrono::hours{24};

    entry.date = start_local_dt;
    entry.start_local = start_local_dt;
    entry.end_local = end_local_dt;
    entry.duration =
      DurationFromDisplayedTimes(entry.start_local, entry.end_local);
    entry.valid = entry.duration.count() >= 0;
    entry.started_too_late = detected.started_too_late;
    entry.ended_too_early = detected.ended_too_early;
  }
};

} // namespace

void
dlgLogbookShowModal() noexcept
{
  TWidgetDialog<LogbookWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Logbook"));
  dialog.SetWidget();
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}
