// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/CutSession.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "util/PrintException.hxx"
#include "util/StaticString.hxx"
#include "TestUtil.hpp"

#include <span>

using namespace std::chrono;

/**
 * Every scenario gets its own directory, so a stale file from an earlier run
 * can never leak into a later assertion.
 */
static AllocatedPath
ScenarioDirectory(const char *name) noexcept
{
  StaticString<64> relative;
  relative.Format("output/test/cutsession-%s", name);

  auto path = AllocatedPath{Path{relative.c_str()}};
  Directory::CreateRecursive(path);
  return path;
}

struct IgcFixture {
  /** IGC HFDTE payload, DDMMYY. */
  const char *date = "040910";

  /** Times of the B-records to write, as HHMMSS. */
  std::span<const char *const> fix_times;

  /** Whether to sign the file, i.e. make it a Closed Session. */
  bool signed_off = false;

  /** Write B-records whose fix-validity flag is 'V' rather than 'A'. */
  bool fixes_invalid = false;

  /** Omit HFFXA, as IGCWriter does for a simulated Session. */
  bool simulated = false;
};

static void
WriteIgc(Path path, const IgcFixture &fixture)
{
  FileOutputStream file{path, FileOutputStream::Mode::CREATE};
  BufferedOutputStream out{file};

  StaticString<128> line;

  out.Write("AXCSFOO\n");
  line.Format("HFDTE%s\n", fixture.date);
  out.Write(line.c_str());

  if (!fixture.simulated)
    /* IGCWriter emits the fix-accuracy record only for a real Session, so
       its absence is what marks a file as simulated */
    out.Write("HFFXA050\n");

  for (const char *time : fixture.fix_times) {
    /* offset 24 carries the fix validity flag, which is what makes a
       B-record count as a valid fix */
    line.Format("B%s5103117N00742367E%c004900048700000\n",
                time, fixture.fixes_invalid ? 'V' : 'A');
    out.Write(line.c_str());
  }

  if (fixture.signed_off)
    for (unsigned i = 0; i < 8; ++i)
      out.Write("G0123456789ABCDEF\n");

  out.Flush();
  file.Commit();
}

static AllocatedPath
WriteIgcInto(Path directory, const char *filename, const IgcFixture &fixture)
{
  auto path = AllocatedPath::Build(directory, Path{filename});
  File::Delete(path);
  WriteIgc(path, fixture);
  return path;
}

static constexpr const char *THREE_FIXES[] = {
  "112238", "112243", "112253",
};

int main()
try {
  plan_tests(34 + 1);

  /* the reference fix: 4 Sep 2010, 11:30:00 UTC, seven minutes after the
     last fix of a file written at 11:22:53 */
  static constexpr BrokenDateTime REFERENCE{2010, 9, 4, 11, 30, 0};

  {
    /* an unsigned file within the bound is a Cut Session */
    const auto dir = ScenarioDirectory("unsigned");
    const auto path = WriteIgcInto(dir, "cut.igc", {.fix_times = THREE_FIXES});

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->path == path);
    ok1(found->b_record_count == 3);
    ok1(found->last_fix_utc == BrokenDateTime(2010, 9, 4, 11, 22, 53));
  }

  {
    /* a signed file is a Closed Session, however recent */
    const auto dir = ScenarioDirectory("signed");
    WriteIgcInto(dir, "closed.igc",
                 {.fix_times = THREE_FIXES, .signed_off = true});

    ok1(!FindCutSession(dir, REFERENCE).has_value());
  }

  {
    /* unsigned but stale: last fix 11:22:53, reference 12:00:00 */
    const auto dir = ScenarioDirectory("stale");
    WriteIgcInto(dir, "old.igc", {.fix_times = THREE_FIXES});

    ok1(!FindCutSession(dir, BrokenDateTime(2010, 9, 4, 12, 0, 0))
        .has_value());
  }

  {
    /* the staleness bound is exactly fifteen minutes */
    const auto dir = ScenarioDirectory("bound");
    WriteIgcInto(dir, "cut.igc", {.fix_times = THREE_FIXES});

    ok1(FindCutSession(dir, BrokenDateTime(2010, 9, 4, 11, 37, 53))
        .has_value());
    ok1(!FindCutSession(dir, BrokenDateTime(2010, 9, 4, 11, 37, 54))
        .has_value());

    /* and it is a parameter, so the caller can tighten or loosen it */
    ok1(!FindCutSession(dir, REFERENCE, minutes{5}).has_value());
    ok1(FindCutSession(dir, REFERENCE, hours{2}).has_value());
  }

  {
    /* a file dated after the current fix means one of the clocks is wrong */
    const auto dir = ScenarioDirectory("future");
    WriteIgcInto(dir, "cut.igc", {.fix_times = THREE_FIXES});

    ok1(!FindCutSession(dir, BrokenDateTime(2010, 9, 4, 11, 0, 0))
        .has_value());
  }

  {
    /* several candidates qualify: the latest last fix wins, and it is the
       last fix that decides, not the file name or the write order */
    static constexpr const char *EARLIER[] = {"110000", "110500"};
    static constexpr const char *LATER[] = {"112000", "112500"};

    const auto dir = ScenarioDirectory("several");
    WriteIgcInto(dir, "zzz-later.igc", {.fix_times = LATER});
    const auto earlier = WriteIgcInto(dir, "aaa-earlier.igc",
                                      {.fix_times = EARLIER});

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->last_fix_utc == BrokenDateTime(2010, 9, 4, 11, 25, 0));
    ok1(found->path != earlier);
  }

  {
    /* a signed file never wins, even when it has the latest last fix */
    static constexpr const char *EARLIER[] = {"112000"};
    static constexpr const char *LATER[] = {"112500"};

    const auto dir = ScenarioDirectory("mixed");
    WriteIgcInto(dir, "signed-later.igc",
                 {.fix_times = LATER, .signed_off = true});
    const auto cut = WriteIgcInto(dir, "cut-earlier.igc",
                                  {.fix_times = EARLIER});

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->path == cut);
    ok1(found->last_fix_utc == BrokenDateTime(2010, 9, 4, 11, 20, 0));
  }

  {
    /* the date comes from HFDTE, not from the file name -- the name's date
       falls back to "today" when the GPS date is unavailable, which is
       exactly the situation after a hard reboot */
    const auto dir = ScenarioDirectory("date-source");
    WriteIgcInto(dir, "2001-01-01-XCS-AAA-01.igc",
                 {.date = "040910", .fix_times = THREE_FIXES});

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->last_fix_utc.GetDate() == BrokenDate(2010, 9, 4));
  }

  {
    /* a file with no header date cannot be placed on a timeline */
    const auto dir = ScenarioDirectory("no-date");
    WriteIgcInto(dir, "undated.igc",
                 {.date = "999999", .fix_times = THREE_FIXES});

    ok1(!FindCutSession(dir, REFERENCE).has_value());
  }

  {
    /* B-records flagged as having no valid GPS fix do not count */
    const auto dir = ScenarioDirectory("invalid-fixes");
    WriteIgcInto(dir, "novalid.igc",
                 {.fix_times = THREE_FIXES, .fixes_invalid = true});

    ok1(!FindCutSession(dir, REFERENCE).has_value());
  }

  {
    /* a header with no fixes at all: the logger opened and died immediately */
    const auto dir = ScenarioDirectory("no-fixes");
    WriteIgcInto(dir, "empty.igc", {});

    ok1(!FindCutSession(dir, REFERENCE).has_value());
  }

  {
    /* A torn final record must not stop the rest of the file being read.
       Truncating it is the appending stage's job; the scan needs only the
       record's time, at offset 1..6, which survives a tear anywhere past the
       fix-validity flag at offset 24.  Counting such a record is right: its
       time is genuine, only trailing extension data was lost. */
    const auto dir = ScenarioDirectory("torn-late");
    const auto path = AllocatedPath::Build(dir, Path{"torn.igc"});
    File::Delete(path);
    {
      FileOutputStream file{path, FileOutputStream::Mode::CREATE};
      BufferedOutputStream out{file};
      out.Write("AXCSFOO\nHFDTE040910\nHFFXA050\n");
      out.Write("B1122385103117N00742367EA004900048700000\n");
      out.Write("B1122435103117N00742367EA0049000487000");
      out.Flush();
      file.Commit();
    }

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->b_record_count == 2);
    ok1(found->last_fix_utc == BrokenDateTime(2010, 9, 4, 11, 22, 43));
  }

  {
    /* torn before the fix-validity flag: the record cannot be read at all,
       so only the intact one counts */
    const auto dir = ScenarioDirectory("torn-early");
    const auto path = AllocatedPath::Build(dir, Path{"torn.igc"});
    File::Delete(path);
    {
      FileOutputStream file{path, FileOutputStream::Mode::CREATE};
      BufferedOutputStream out{file};
      out.Write("AXCSFOO\nHFDTE040910\nHFFXA050\n");
      out.Write("B1122385103117N00742367EA004900048700000\n");
      out.Write("B11224351031");
      out.Flush();
      file.Commit();
    }

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->b_record_count == 1);
    ok1(found->last_fix_utc == BrokenDateTime(2010, 9, 4, 11, 22, 38));
  }

  {
    /* files that are not IGC are ignored */
    const auto dir = ScenarioDirectory("other-files");
    WriteIgcInto(dir, "cut.igc", {.fix_times = THREE_FIXES});
    WriteIgcInto(dir, "notes.txt", {.fix_times = THREE_FIXES});

    const auto found = FindCutSession(dir, REFERENCE);
    ok1(found.has_value());
    ok1(found->path.GetBase() == Path{"cut.igc"});
  }

  {
    /* a simulated Session is never signed, so it would qualify for ever;
       appending real fixes to it would put a genuine Flight inside a
       simulator file */
    const auto dir = ScenarioDirectory("simulated");
    WriteIgcInto(dir, "sim.igc",
                 {.fix_times = THREE_FIXES, .simulated = true});

    ok1(!FindCutSession(dir, REFERENCE).has_value());
  }

  {
    /* an empty directory, and one that does not exist at all */
    const auto dir = ScenarioDirectory("empty-dir");
    ok1(!FindCutSession(dir, REFERENCE).has_value());
    ok1(!FindCutSession(Path{"output/test/cutsession-does-not-exist"},
                        REFERENCE).has_value());
  }

  {
    /* without a plausible GPS date and time there is nothing to compare
       against, and the system clock is not an acceptable substitute */
    const auto dir = ScenarioDirectory("no-reference");
    WriteIgcInto(dir, "cut.igc", {.fix_times = THREE_FIXES});

    ok1(!FindCutSession(dir, BrokenDateTime::Invalid()).has_value());
    ok1(!FindCutSession(dir, BrokenDateTime(BrokenDate::Invalid(),
                                            BrokenTime(11, 30, 0)))
        .has_value());
  }

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
