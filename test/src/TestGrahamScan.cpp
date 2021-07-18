/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
