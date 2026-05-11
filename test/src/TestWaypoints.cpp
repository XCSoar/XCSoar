// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoint/Waypoints.hpp"
#include "Geo/GeoVector.hpp"
#include "test_debug.hpp"
#include "util/Macros.hpp"

#include <functional>

#include <stdio.h>
extern "C" {
#include "tap.h"
}

class WaypointPredicateCounter
{
public:
  typedef std::function<bool(const Waypoint &wp)> Predicate;

private:
  Predicate predicate;
  unsigned count;

public:
  WaypointPredicateCounter(const Predicate &_predicate)
    :predicate(_predicate), count(0) {}

  void Visit(const WaypointPtr &wp) noexcept {
    if (predicate(*wp))
      count++;
  }

  unsigned GetCounter() const {
    return count;
  }
};

static void
AddSpiralWaypoints(Waypoints &waypoints,
                   const GeoPoint &center = GeoPoint(Angle::Degrees(51.4),
                                                     Angle::Degrees(7.85)),
                   Angle angle_start = Angle::Degrees(0),
                   Angle angle_step = Angle::Degrees(15),
                   double distance_start = 0,
                   double distance_step = 1000,
                   double distance_max = 150000)
{
  assert(distance_step > 0);

  for (unsigned i = 0;; ++i) {
    GeoVector vector;
    vector.distance = distance_start + distance_step * i;
    if (vector.distance > distance_max)
      break;

    vector.bearing = angle_start + angle_step * i;

    Waypoint waypoint{vector.EndPoint(center)};
    waypoint.original_id = i;
    waypoint.elevation = i * 10 - 500;
    waypoint.has_elevation = true;

    StaticString<256> buffer;

    if (i % 7 == 0) {
      buffer = "Airfield";
      waypoint.type = Waypoint::Type::AIRFIELD;
    } else if (i % 3 == 0) {
      buffer = "Field";
      waypoint.type = Waypoint::Type::OUTLANDING;
    } else
      buffer = "Waypoint";

    buffer.AppendFormat(" #%d", i + 1);
    waypoint.name = buffer;

    waypoints.Append(std::move(waypoint));
  }

  waypoints.Optimise();
}

static void
TestLookups(const Waypoints &waypoints, const GeoPoint &center)
{
  WaypointPtr waypoint;

  ok1((waypoint = waypoints.LookupId(0)) == NULL);
  ok1((waypoint = waypoints.LookupId(1)) != NULL);
  ok1(waypoint->original_id == 0);
  ok1((waypoint = waypoints.LookupId(151)) != NULL);
  ok1(waypoint->original_id == 150);
  ok1((waypoint = waypoints.LookupId(152)) == NULL);
  ok1((waypoint = waypoints.LookupId(160)) == NULL);

  ok1((waypoint = waypoints.LookupLocation(center, 0)) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.LookupName("Waypoint #5")) != NULL);
  ok1(waypoint->original_id == 4);

  ok1((waypoint = waypoints.LookupLocation(waypoint->location, 10000)) != NULL);
  ok1(waypoint->original_id == 4);
}

class BeginsWith
{
  const char *prefix;

public:
  BeginsWith(const char *_prefix):prefix(_prefix) {}

  bool operator()(const Waypoint &waypoint) {
    return StringStartsWith(waypoint.name.c_str(), prefix);
  }
};

static void
TestNamePrefixVisitor(const Waypoints &waypoints, const char *prefix,
                      unsigned expected_results)
{
  WaypointPredicateCounter::Predicate predicate = BeginsWith(prefix);
  WaypointPredicateCounter prefix_counter(predicate);
  waypoints.VisitNamePrefix(prefix, [&](const auto &wp){ prefix_counter.Visit(wp); });
  ok1(prefix_counter.GetCounter() == expected_results);
}

static void
TestNamePrefixVisitor(const Waypoints &waypoints)
{
  TestNamePrefixVisitor(waypoints, "", 151);
  TestNamePrefixVisitor(waypoints, "Foo", 0);
  TestNamePrefixVisitor(waypoints, "a", 0);
  TestNamePrefixVisitor(waypoints, "A", 22);
  TestNamePrefixVisitor(waypoints, "Air", 22);
  TestNamePrefixVisitor(waypoints, "Field", 51 - 8);
}

static void
TestNameSubstringVisitor(const Waypoints &waypoints, const char *substring,
                         unsigned expected_results)
{
  unsigned count = 0;
  waypoints.VisitNameSubstring(substring,
                               [&]([[maybe_unused]] const auto &wp){
                                 ++count;
                               });
  ok1(count == expected_results);
}

static void
TestNameSubstringVisitor(const Waypoints &waypoints)
{
  /* Spiral has 22 "Airfield", 51-8=43 "Field", 151-22-43=86
     "Waypoint" entries.  Letter A appears in Airfield AND
     Waypoint; W and POINT appear only in Waypoint; FIELD
     appears in both Airfield and Field. */

  /* Empty pattern matches every waypoint. */
  TestNameSubstringVisitor(waypoints, "", 151);

  /* Non-existent substring. */
  TestNameSubstringVisitor(waypoints, "Foo", 0);

  /* Case-insensitive: "a" matches "Airfield" + "Waypoint". */
  TestNameSubstringVisitor(waypoints, "a", 22 + 86);
  TestNameSubstringVisitor(waypoints, "A", 22 + 86);

  /* "Air" only in Airfield. */
  TestNameSubstringVisitor(waypoints, "Air", 22);

  /* Substring (not just prefix): "Field" matches both "Field"
     and "Airfield" entries. */
  TestNameSubstringVisitor(waypoints, "Field", 22 + 43);
  TestNameSubstringVisitor(waypoints, "field", 22 + 43);

  /* "Point" appears only inside "Waypoint". */
  TestNameSubstringVisitor(waypoints, "Point", 86);

  /* Non-alphanumerics are dropped from both sides by
     NormalizeSearchString, so " #" normalises to empty and
     therefore matches every waypoint. */
  TestNameSubstringVisitor(waypoints, " #", 151);
}

class CloserThan
{
  double distance;
  GeoPoint location;

public:
  CloserThan(double _distance, const GeoPoint &_location)
    :distance(_distance), location(_location) {}

  bool operator()(const Waypoint &waypoint) {
    return location.Distance(waypoint.location) < distance;
  }
};

static void
TestRangeVisitor(const Waypoints &waypoints, const GeoPoint &location,
                 double distance, unsigned expected_results)
{
  WaypointPredicateCounter::Predicate predicate = CloserThan(distance, location);
  WaypointPredicateCounter distance_counter(predicate);
  waypoints.VisitWithinRange(location, distance, [&](const auto &wp){ distance_counter.Visit(wp); });
  ok1(distance_counter.GetCounter() == expected_results);
}

static void
TestRangeVisitor(const Waypoints &waypoints, const GeoPoint &center)
{
  TestRangeVisitor(waypoints, center, 1, 1);
  TestRangeVisitor(waypoints, center, 999, 1);
  TestRangeVisitor(waypoints, center, 1300, 2);
  TestRangeVisitor(waypoints, center, 10500, 11);
  TestRangeVisitor(waypoints, center, 1000000, 151);
}

static bool
OriginalIDAbove5(const Waypoint &waypoint) {
  return waypoint.original_id > 5;
}

static void
TestGetNearest(const Waypoints &waypoints, const GeoPoint &center)
{
  WaypointPtr waypoint;
  GeoPoint near = GeoVector(250, Angle::Degrees(15)).EndPoint(center);
  GeoPoint far = GeoVector(750, Angle::Degrees(15)).EndPoint(center);
  GeoPoint further = GeoVector(4200, Angle::Degrees(48)).EndPoint(center);

  ok1((waypoint = waypoints.GetNearest(center, 1)) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearest(center, 10000)) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearest(near, 1)) == NULL);

  ok1((waypoint = waypoints.GetNearest(near, 10000)) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearest(far, 1)) == NULL);

  ok1((waypoint = waypoints.GetNearest(far, 10000)) != NULL);
  ok1(waypoint->original_id == 1);

  ok1((waypoint = waypoints.GetNearestLandable(center, 1)) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearestLandable(center, 10000)) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearestLandable(further, 1)) == NULL);

  ok1((waypoint = waypoints.GetNearestLandable(further, 10000)) != NULL);
  ok1(waypoint->original_id == 3);

  ok1((waypoint = waypoints.GetNearestIf(center, 1, OriginalIDAbove5)) == NULL);

  ok1((waypoint = waypoints.GetNearestIf(center, 10000, OriginalIDAbove5)) != NULL);
  ok1(waypoint->original_id == 6);
}

static void
TestIterator(const Waypoints &waypoints)
{
  unsigned count = 0;
  for (const auto &i : waypoints) {
    count++;
    (void)i;
  }

  ok1(count == 151);
}

static unsigned
TestCopy(Waypoints& waypoints)
{
  const WaypointPtr wp = waypoints.LookupId(5);
  if (!wp)
    return false;

  unsigned size_old = waypoints.size();
  Waypoint wp_copy = *wp;
  wp_copy.id = waypoints.size() + 1;
  waypoints.Append(std::move(wp_copy));
  waypoints.Optimise();
  unsigned size_new = waypoints.size();
  return (size_new == size_old + 1);
}

static bool
TestErase(Waypoints& waypoints, unsigned id)
{
  waypoints.Optimise();
  auto wp = waypoints.LookupId(id);
  if (wp == NULL)
    return false;

  waypoints.Erase(std::move(wp));
  waypoints.Optimise();

  wp = waypoints.LookupId(id);
  return wp == NULL;
}

static bool
TestReplace(Waypoints& waypoints, unsigned id)
{
  auto wp = waypoints.LookupId(id);
  if (wp == NULL)
    return false;

  std::string oldName = wp->name;

  Waypoint copy = *wp;
  copy.name = "Fred";
  waypoints.Replace(wp, std::move(copy));
  waypoints.Optimise();

  wp = waypoints.LookupId(id);
  return wp != NULL && wp->name != oldName && wp->name == "Fred";
}

static bool
SuggestionContains(const char *suggestion, char ch) noexcept
{
  if (suggestion == nullptr)
    return false;
  for (const char *p = suggestion; *p; ++p)
    if (*p == ch)
      return true;
  return false;
}

static void
TestSuggestNameSubstring()
{
  /* Tiny waypoint set with three distinct second letters after
     the substring "DI": "DITTINGEN" -> 'T', "DIANA" -> 'A',
     "ADIEU" -> 'E' (substring match inside the name).  No 'X'
     anywhere.  We also include a shortname-only match. */
  Waypoints waypoints;

  Waypoint w1{GeoPoint(Angle::Degrees(7.0), Angle::Degrees(47.0))};
  w1.name = "DITTINGEN";
  waypoints.Append(std::move(w1));

  Waypoint w2{GeoPoint(Angle::Degrees(7.1), Angle::Degrees(47.1))};
  w2.name = "DIANA";
  waypoints.Append(std::move(w2));

  Waypoint w3{GeoPoint(Angle::Degrees(7.2), Angle::Degrees(47.2))};
  w3.name = "ADIEU";
  waypoints.Append(std::move(w3));

  Waypoint w4{GeoPoint(Angle::Degrees(7.3), Angle::Degrees(47.3))};
  w4.name = "Plain";
  w4.shortname = "DIVE";
  waypoints.Append(std::move(w4));

  waypoints.Optimise();

  char buffer[64];

  /* Empty input => union of all distinct chars across all
     names/shortnames.  Should include letters present in the
     data and exclude ones that aren't (e.g. 'X', 'Q'). */
  const char *all = waypoints.SuggestNameSubstring("", buffer,
                                                   ARRAY_SIZE(buffer));
  ok1(all != nullptr);
  ok1(SuggestionContains(all, 'D'));
  ok1(SuggestionContains(all, 'I'));
  ok1(SuggestionContains(all, 'V'));      // only via shortname "DIVE"
  ok1(!SuggestionContains(all, 'X'));
  ok1(!SuggestionContains(all, 'Q'));

  /* "DI" => candidates that follow it anywhere in any name/shortname.
     Expect: T (DITTINGEN), A (DIANA), E (ADIEU), V (DIVE).
     Should not include 'Z'. */
  const char *after_di = waypoints.SuggestNameSubstring("DI", buffer,
                                                        ARRAY_SIZE(buffer));
  ok1(after_di != nullptr);
  ok1(SuggestionContains(after_di, 'T'));
  ok1(SuggestionContains(after_di, 'A'));
  ok1(SuggestionContains(after_di, 'E'));
  ok1(SuggestionContains(after_di, 'V'));
  ok1(!SuggestionContains(after_di, 'Z'));

  /* Lowercase input is normalised (uppercased) before scanning. */
  const char *after_di_lower = waypoints.SuggestNameSubstring("di", buffer,
                                                              ARRAY_SIZE(buffer));
  ok1(after_di_lower != nullptr);
  ok1(SuggestionContains(after_di_lower, 'T'));

  /* Pattern that doesn't match anything => nullptr (keyboard
     should re-enable all keys so the user can correct). */
  ok1(waypoints.SuggestNameSubstring("ZZZZ", buffer,
                                     ARRAY_SIZE(buffer)) == nullptr);

  /* Pattern that matches only at the very end of a name returns
     a non-null string (any_match=true) but with no "next"
     characters from that occurrence.  "EU" matches at end of
     ADIEU; nothing follows.  But the result is still non-null
     because we did find a match -- the user can still backspace.
     The empty allowed set means no key is enabled, which is
     correct: nothing extends the pattern. */
  const char *after_eu = waypoints.SuggestNameSubstring("EU", buffer,
                                                        ARRAY_SIZE(buffer));
  ok1(after_eu != nullptr);
  ok1(after_eu[0] == '\0');
}

static void
TestNameSubstringShortname()
{
  /* Reproduce the SeeYou .cup case where the long name is the
     human-readable airport name and the shortname is the ICAO
     code (e.g. name="DITTINGEN 'R'", shortname="LSPD"). */
  Waypoints waypoints;

  Waypoint dittingen{GeoPoint(Angle::Degrees(7.49), Angle::Degrees(47.44))};
  dittingen.name = "DITTINGEN 'R'";
  dittingen.shortname = "LSPD";
  waypoints.Append(std::move(dittingen));

  Waypoint ambri{GeoPoint(Angle::Degrees(8.69), Angle::Degrees(46.51))};
  ambri.name = "AMBRI";
  ambri.shortname = "LSPM";
  waypoints.Append(std::move(ambri));

  /* Waypoint whose long name AND shortname both contain the
     same substring -- the regression check for #1316. */
  Waypoint dual{GeoPoint(Angle::Degrees(7.00), Angle::Degrees(46.00))};
  dual.name = "ALPHA FIELD";
  dual.shortname = "ALPHA";
  waypoints.Append(std::move(dual));

  waypoints.Optimise();

  /* Lookup by long-name substring still works. */
  unsigned name_hits = 0;
  waypoints.VisitNameSubstring("DITT",
                               [&]([[maybe_unused]] const auto &wp){
                                 ++name_hits;
                               });
  ok1(name_hits == 1);

  /* Lookup by ICAO shortname (lowercase user input). */
  unsigned shortname_hits = 0;
  WaypointPtr matched;
  waypoints.VisitNameSubstring("lspd",
                               [&](const auto &wp){
                                 ++shortname_hits;
                                 matched = wp;
                               });
  ok1(shortname_hits == 1);
  ok1(matched && matched->shortname == "LSPD");

  /* Common prefix of shortnames matches both. */
  unsigned prefix_hits = 0;
  waypoints.VisitNameSubstring("LSP",
                               [&]([[maybe_unused]] const auto &wp){
                                 ++prefix_hits;
                               });
  ok1(prefix_hits == 2);

  /* A single waypoint whose name AND shortname both contain
     the search substring must be visited exactly once.  The
     pre-substring radix-tree path inserted each waypoint twice
     (once under its normalised name, once under its normalised
     shortname) and would have produced 2 hits here.  Regression
     test for #1316. */
  unsigned dual_hits = 0;
  WaypointPtr dual_matched;
  waypoints.VisitNameSubstring("ALPHA",
                               [&](const auto &wp){
                                 ++dual_hits;
                                 dual_matched = wp;
                               });
  ok1(dual_hits == 1);
  ok1(dual_matched && dual_matched->name == "ALPHA FIELD");
}

int
main(int argc, char** argv)
{
  if (!ParseArgs(argc, argv))
    return 0;

  plan_tests(52 + 9 + 6 + 17);

  Waypoints waypoints;
  GeoPoint center(Angle::Degrees(51.4), Angle::Degrees(7.85));

  // AddSpiralWaypoints creates 151 waypoints from
  // 0km to 150km distance in 1km steps
  AddSpiralWaypoints(waypoints, center);

  ok1(!waypoints.IsEmpty());
  ok1(waypoints.size() == 151);

  TestLookups(waypoints, center);
  TestNamePrefixVisitor(waypoints);
  TestNameSubstringVisitor(waypoints);
  TestNameSubstringShortname();
  TestSuggestNameSubstring();
  TestRangeVisitor(waypoints, center);
  TestGetNearest(waypoints, center);
  TestIterator(waypoints);

  ok(TestCopy(waypoints), "waypoint copy", 0);
  ok(TestErase(waypoints, 3), "waypoint erase", 0);
  ok(TestReplace(waypoints, 4), "waypoint replace", 0);

  // test clear
  waypoints.Clear();
  ok1(waypoints.IsEmpty());
  ok1(waypoints.size() == 0);

  return exit_status();
}
