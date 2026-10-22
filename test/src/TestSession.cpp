// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Session/SessionState.hpp"
#include "system/Path.hpp"

#include "TestUtil.hpp"

#include <stdio.h>

static bool
WriteRaw(Path path, const char *text) noexcept
{
  FILE *file = fopen(path.c_str(), "w");
  if (file == nullptr)
    return false;

  fputs(text, file);
  fclose(file);
  return true;
}

static void
TestRoundTrip()
{
  SessionState state;
  state.flying = true;
  state.have_ballast = true;
  state.ballast_litres = 120;
  state.have_crew = true;
  state.crew_mass = 95;
  state.have_bugs = true;
  state.bugs = 0.98;
  state.have_qnh = true;
  state.qnh_hpa = 1015;
  state.have_max_temp = true;
  state.max_temp_k = 300;
  state.have_atc_reference = true;
  state.atc_reference = GeoPoint(Angle::Degrees(8.5),
                                 Angle::Degrees(47.4));
  state.have_atc_declination = true;
  state.atc_declination_deg = -2;
  state.have_goto = true;
  state.goto_id = 42;
  state.have_goto_location = true;
  state.goto_location = GeoPoint(Angle::Degrees(8.51),
                                 Angle::Degrees(47.49));
  state.have_goto_elevation = true;
  state.goto_elevation = 450;
  state.goto_name = "(goto)";
  state.checklist_file = "checklists/xcsoar-checklist.txt";
  state.checklist_page = 1;
  state.checklist[0].present = true;
  state.checklist[0].sum = SessionHash("page");
  state.checklist[0].ticks = "1010";

  const Path path("output/TestSession.txt");
  SaveSession(state, path);

  SessionState loaded;
  LoadSession(loaded, path);

  ok1(loaded.flying);
  ok1(equals(loaded.ballast_litres, 120));
  ok1(equals(loaded.crew_mass, 95));
  ok1(equals(loaded.bugs, 0.98));
  ok1(loaded.have_qnh);
  ok1(equals(loaded.qnh_hpa, 1015));
  ok1(loaded.have_max_temp);
  ok1(equals(loaded.max_temp_k, 300));
  ok1(loaded.have_atc_reference);
  ok1(equals(loaded.atc_reference.longitude.Degrees(), 8.5));
  ok1(equals(loaded.atc_reference.latitude.Degrees(), 47.4));
  ok1(equals(loaded.atc_declination_deg, -2));
  ok1(loaded.ShouldRestoreGoto());
  ok1(loaded.goto_id == 42);
  ok1(loaded.goto_name == "(goto)");
  ok1(equals(loaded.goto_elevation, 450));
  ok1(loaded.checklist_page == 1);
  ok1(loaded.checklist_file == "checklists/xcsoar-checklist.txt");

  std::string ticks;
  ok1(loaded.ChecklistTicks(0, SessionHash("page"), ticks));
  ok1(ticks == "1010");
  ok1(!loaded.modified);
  ok1(!loaded.seen_flying);
}

static void
TestFlyingClearIgnoresPolar()
{
  ok1(WriteRaw(Path("output/TestSessionFlying.txt"),
               "Flying=\"0\"\n"
               "BallastLitres=\"120\"\n"
               "CrewMass=\"95\"\n"
               "Bugs=\"0.98\"\n"));

  SessionState loaded;
  LoadSession(loaded, Path("output/TestSessionFlying.txt"));
  ok1(!loaded.flying);
  ok1(loaded.have_ballast);
  ok1(equals(loaded.ballast_litres, 120));
}

static void
TestBadGeoPoint()
{
  ok1(WriteRaw(Path("output/TestSessionGeo.txt"),
               "AtcReference=\"999 10\"\n"));

  SessionState loaded;
  LoadSession(loaded, Path("output/TestSessionGeo.txt"));
  ok1(!loaded.have_atc_reference);
  ok1(!loaded.atc_reference.IsValid());
}

static void
TestChecklistMismatch()
{
  SessionState state;
  state.checklist[0].present = true;
  state.checklist[0].sum = SessionHash("one");
  state.checklist[0].ticks = "11";

  std::string ticks;
  ok1(!state.ChecklistTicks(0, SessionHash("two"), ticks));
  ok1(state.ChecklistTicks(0, SessionHash("one"), ticks));
  ok1(ticks == "11");
  ok1(SessionHash("a") == SessionHash("a"));
  ok1(SessionHash("a") != SessionHash("b"));
}

static void
TestTakeoffGoto()
{
  SessionState state;
  state.have_goto = true;
  state.have_goto_location = true;
  state.goto_location = GeoPoint(Angle::Degrees(1), Angle::Degrees(2));
  state.goto_name = "(takeoff)";
  ok1(!state.ShouldRestoreGoto());

  state.goto_name = "(goto)";
  ok1(state.ShouldRestoreGoto());
}

static void
TestFlyingLatch()
{
  SessionState state;
  state.flying = true;
  state.have_ballast = true;
  state.ballast_litres = 40;

  ok1(!state.NoteFlying(false, 0, 90, 1));
  ok1(state.flying);
  ok1(equals(state.ballast_litres, 40));

  ok1(state.NoteFlying(true, 10, 80, 0.9));
  ok1(state.seen_flying);
  ok1(state.flying);
  ok1(equals(state.ballast_litres, 10));
  ok1(equals(state.crew_mass, 80));
  ok1(equals(state.bugs, 0.9));

  ok1(state.NoteFlying(false, 10, 80, 0.9));
  ok1(!state.flying);
  ok1(!state.NoteFlying(false, 0, 90, 1));
}

int
main()
{
  plan_tests(22 + 4 + 3 + 5 + 2 + 12);

  TestRoundTrip();
  TestFlyingClearIgnoresPolar();
  TestBadGeoPoint();
  TestChecklistMismatch();
  TestTakeoffGoto();
  TestFlyingLatch();

  return exit_status();
}
