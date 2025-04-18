// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/SearchPointVector.hpp"
#include "TestUtil.hpp"

static constexpr GeoPoint
GP(double longitude, double latitude) noexcept
{
  return GeoPoint{Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

static SearchPoint
SP(double longitude, double latitude)
{
  return SearchPoint(GP(longitude, latitude));
}

static bool
Contains(const SearchPointVector &v, const SearchPoint &sp) noexcept
{
  for (const auto &i : v)
    if (i.GetLocation() == sp.GetLocation())
      return true;

  return false;
}

static void
TestNotPruned()
{
  SearchPointVector v;
  v.emplace_back(SP(100, 100));
  v.emplace_back(SP(100, 105));
  v.emplace_back(SP(105, 105));
  v.emplace_back(SP(105, 100));

  ok1(!v.PruneInterior());
  ok1(v.size() == 4);
}

static void
TestPruned1()
{
  SearchPointVector v;
  v.emplace_back(SP(100, 100));
  v.emplace_back(SP(100, 105));
  v.emplace_back(SP(105, 105));
  v.emplace_back(SP(105, 100));
  v.emplace_back(SP(102, 104));

  ok1(v.PruneInterior());
  ok1(v.size() == 4);
  ok1(!Contains(v, SP(102, 104)));
}

/**
 * Test for https://github.com/XCSoar/XCSoar/issues/259 fixed by
 * commit 3588f57e253
 */
static void
TestPruned2()
{
  SearchPointVector v;
  v.emplace_back(SP(153, 150));
  v.emplace_back(SP(23, 18));
  v.emplace_back(SP(40, 17));
  v.emplace_back(SP(154, 149));

  ok1(!v.PruneInterior());
}

int
main()
{
  plan_tests(6);

  TestNotPruned();
  TestPruned1();
  TestPruned2();
}
