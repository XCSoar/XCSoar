// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/IGCWriter.hpp"
#include "system/FileUtil.hpp"
#include "NMEA/Info.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "TestUtil.hpp"
#include "util/PrintException.hxx"

#include <cassert>
#include <cstdio>
#include <string_view>

static void
CheckTextFile(Path path, const char *const* expect)
{
  FileLineReaderA reader(path);

  const char *line;
  while ((line = reader.ReadLine()) != NULL) {
    if (*line == 'G')
      break;

    ok1(*expect != NULL);

    if (StringIsEqual(*expect, "HFFTYFRTYPE:", 12)) {
      ok1(StringIsEqual(line, "HFFTYFRTYPE:", 12));
    } else {
      if (strcmp(line, *expect)) {
        printf("# \"%s\" fails to match with \"%s\"\n", line, *expect);
      }
      ok1(strcmp(line, *expect) == 0);
    }

    ++expect;
  }

  ok1(*expect == NULL);
}

static const char *const expect[] = {
  "AXCSFOO",
  "HFDTE040910",
  "HFFXA050",
  "HFPLTPILOTINCHARGE:Pilot Name",
  "HFCM2CREW2:CoPilot Name",
  "HFGTYGLIDERTYPE:ASK-21",
  "HFGIDGLIDERID:D-1234",
  "HFCIDCOMPETITIONID:34",
  "HFFTYFRTYPE:XCSOAR XCSOAR",
  "HFGPS:bar",
  "HFDTM100DATUM:WGS-1984",
  "I023638FXA3940SIU",
  "C040910112233000000000001",
  "C0000000N00000000ETAKEOFF",
  "C5103117N00742367EBERGNEUSTADT",
  "C5037932N01043567ESUHL",
  "C5103117N00742367EBERGNEUSTADT",
  "C0000000N00000000ELANDING",
  "F112233",
  "B1122385103117N00742367EA004900048700000",
  "E112243my_event",
  "B1122435103117N00742367EA004900048700000",
  "LPLTmy_note",
  "F112253121701",
  "B1122535103117S00742367WA004900048700000",
  NULL
};

static void
Run(IGCWriter &writer)
{
  static const GeoPoint home(Angle::Degrees(7.7061111111111114),
                             Angle::Degrees(51.051944444444445));
  static const GeoPoint tp(Angle::Degrees(10.726111111111111),
                           Angle::Degrees(50.6322));

  static NMEAInfo i;
  i.clock = i.time = TimeStamp{std::chrono::seconds{1}};
  i.time_available.Update(i.clock);
  i.date_time_utc.year = 2010;
  i.date_time_utc.month = 9;
  i.date_time_utc.day = 4;
  i.date_time_utc.hour = 11;
  i.date_time_utc.minute = 22;
  i.date_time_utc.second = 33;
  i.location = home;
  i.location_available.Update(i.clock);
  i.gps_altitude = 487;
  i.gps_altitude_available.Update(i.clock);
  i.ProvidePressureAltitude(490);
  i.ProvideBaroAltitudeTrue(400);

  writer.WriteHeader(i.date_time_utc, "Pilot Name", "CoPilot Name", "ASK-21",
                     "D-1234", "34", "FOO", "bar", false);
  writer.StartDeclaration(i.date_time_utc, 3);
  writer.AddDeclaration(home, "Bergneustadt");
  writer.AddDeclaration(tp, "Suhl");
  writer.AddDeclaration(home, "Bergneustadt");
  writer.EndDeclaration();

  writer.LogEmptyFRecord(i.date_time_utc);

  i.date_time_utc.second += 5;
  writer.LogPoint(i);
  i.date_time_utc.second += 5;
  writer.LogEvent(i, "my_event");
  i.date_time_utc.second += 5;
  writer.LoggerNote("my_note");

  int satellites[GPSState::MAXSATELLITES];
  for (unsigned i = 0; i < GPSState::MAXSATELLITES; ++i)
    satellites[i] = 0;

  satellites[2] = 12;
  satellites[4] = 17;
  satellites[7] = 1;

  i.date_time_utc.second += 5;
  writer.LogFRecord(i.date_time_utc, satellites);

  i.location = GeoPoint(Angle::Degrees(-7.7061111111111114),
                        Angle::Degrees(-51.051944444444445));
  writer.LogPoint(i);

  writer.Flush();
  writer.Sign();
  writer.Flush();
}

static void
Run(Path path)
{
  IGCWriter writer(path);
  Run(writer);
}

/**
 * Write the first half of a Logger Session and abandon it without signing,
 * exactly as a crash or a power cut would leave it.
 */
static void
RunCutSession(Path path)
{
  static const GeoPoint home(Angle::Degrees(7.7061111111111114),
                             Angle::Degrees(51.051944444444445));

  IGCWriter writer(path);

  static NMEAInfo i;
  i.clock = i.time = TimeStamp{std::chrono::seconds{1}};
  i.time_available.Update(i.clock);
  i.date_time_utc = BrokenDateTime(2010, 9, 4, 11, 22, 33);
  i.location = home;
  i.location_available.Update(i.clock);
  i.gps_altitude = 487;
  i.gps_altitude_available.Update(i.clock);
  i.ProvidePressureAltitude(490);
  i.ProvideBaroAltitudeTrue(400);

  writer.WriteHeader(i.date_time_utc, "Pilot Name", "CoPilot Name", "ASK-21",
                     "D-1234", "34", "FOO", "bar", false);
  writer.StartDeclaration(i.date_time_utc, 1);
  writer.AddDeclaration(home, "Bergneustadt");
  writer.EndDeclaration();

  for (unsigned n = 0; n < 3; ++n) {
    i.date_time_utc.second += 5;
    writer.LogPoint(i);
  }

  writer.Flush();
  /* deliberately no Sign(): a Cut Session is unsigned */
}

/** Count the lines of a file that start with the given record type. */
static unsigned
CountRecords(Path path, char type)
{
  FileLineReaderA reader(path);

  unsigned count = 0;
  const char *line;
  while ((line = reader.ReadLine()) != nullptr)
    if (line[0] == type)
      ++count;

  return count;
}

static void
AppendBytes(Path path, std::string_view bytes)
{
  FileOutputStream file(path, FileOutputStream::Mode::APPEND_EXISTING);
  BufferedOutputStream out(file);
  out.Write(bytes);
  out.Flush();
  file.Commit();
}

/**
 * A Flight interrupted by a crash stays one file: the second Logger Session
 * continues the first rather than starting a new one.
 */
static void
TestAppend()
{
  const Path path("output/test/test-append.igc");
  File::Delete(path);

  RunCutSession(path);
  const unsigned fixes_before = CountRecords(path, 'B');
  const unsigned headers_before = CountRecords(path, 'H');
  ok1(fixes_before == 3);
  ok1(CountRecords(path, 'G') == 0);

  {
    IGCWriter writer(path, IGCWriter::Mode::APPEND);

    static NMEAInfo i;
    i.clock = i.time = TimeStamp{std::chrono::seconds{1}};
    i.time_available.Update(i.clock);
    i.date_time_utc = BrokenDateTime(2010, 9, 4, 11, 30, 0);
    i.location = GeoPoint(Angle::Degrees(7.7), Angle::Degrees(51.05));
    i.location_available.Update(i.clock);
    i.gps_altitude = 900;
    i.gps_altitude_available.Update(i.clock);
    i.ProvidePressureAltitude(910);
    i.ProvideBaroAltitudeTrue(880);

    for (unsigned n = 0; n < 2; ++n) {
      i.date_time_utc.second += 5;
      writer.LogPoint(i);
    }

    writer.Flush();
    writer.Sign();
    writer.Flush();
  }

  /* one Flight, one file: the fixes from both Sessions, and exactly one
     header and one declaration.  The declaration of a single turnpoint is
     four C-records -- its header, TAKEOFF, the turnpoint, LANDING -- so
     eight would mean it had been written a second time. */
  ok1(CountRecords(path, 'B') == fixes_before + 2);
  ok1(CountRecords(path, 'A') == 1);
  ok1(CountRecords(path, 'C') == 4);
  /* exactly as many H-records as before: StartLogger skips the whole header
     block when appending, and ">0" would pass even if it wrote it twice */
  ok1(CountRecords(path, 'H') == headers_before);

  /* the signature covers the records written before the cut, not just the
     ones written after it */
  GRecord grecord;
  grecord.Initialize();
  grecord.VerifyGRecordInFile(path);
  ok1(true);
}

/**
 * A power cut can stop mid-record.  The fragment must not survive into the
 * middle of the file, where it would be signed as though it were a record.
 */
static void
TestTornFinalRecord()
{
  const Path path("output/test/test-torn.igc");
  File::Delete(path);

  RunCutSession(path);
  AppendBytes(path, "B1122535103117N0074236");

  {
    IGCWriter writer(path, IGCWriter::Mode::APPEND);
    writer.Flush();
    writer.Sign();
    writer.Flush();
  }

  /* the fragment is gone and the three intact fixes remain */
  ok1(CountRecords(path, 'B') == 3);

  GRecord grecord;
  grecord.Initialize();
  grecord.VerifyGRecordInFile(path);
  ok1(true);
}

/**
 * Damage anywhere but the very end cannot be repaired by truncating, so the
 * file is refused and the logger is left to start a new one.
 */
static void
TestMidFileCorruption()
{
  const Path path("output/test/test-corrupt.igc");
  File::Delete(path);

  RunCutSession(path);
  /* a block the filesystem never wrote, followed by more valid records */
  AppendBytes(path, std::string_view{"\0\0\0\0\0\0\0\0", 8});
  AppendBytes(path, "\nB1122535103117N00742367EA004900048700000\n");

  bool refused = false;
  try {
    IGCWriter writer(path, IGCWriter::Mode::APPEND);
  } catch (...) {
    refused = true;
  }

  ok1(refused);
}

/**
 * An empty or record-less file is not something to continue.
 */
static void
TestNothingToContinue()
{
  const Path path("output/test/test-empty.igc");
  File::Delete(path);

  {
    FileOutputStream file(path, FileOutputStream::Mode::CREATE);
    file.Commit();
  }

  bool refused = false;
  try {
    IGCWriter writer(path, IGCWriter::Mode::APPEND);
  } catch (...) {
    refused = true;
  }

  ok1(refused);
}

int main()
try {
  plan_tests(51 + 11);

  const Path path("output/test/test.igc");
  File::Delete(path);

  Run(path);

  CheckTextFile(path, expect);

  GRecord grecord;
  grecord.Initialize();
  grecord.VerifyGRecordInFile(path);

  TestAppend();
  TestTornFinalRecord();
  TestMidFileCorruption();
  TestNothingToContinue();

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
