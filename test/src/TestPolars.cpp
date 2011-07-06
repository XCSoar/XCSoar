/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "TestUtil.hpp"

#include "GlideSolvers/GlidePolar.hpp"
#include "IO/ConfiguredFile.hpp"
#include "Profile/Profile.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Polar/PolarStore.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

void GlidePolar::Update() {}

TLineReader*
OpenConfiguredTextFile(const TCHAR *profile_key, ConvertLineReader::charset cs)
{
  return NULL;
}

static void
TestBasic()
{
  // Test ReadString()
  PolarInfo polar;
  polar.ReadString(_T("318, 100, 80, -0.606, 120, -0.99, 160, -1.918"));
  ok1(equals(fixed(polar.reference_mass), 318));
  ok1(equals(fixed(polar.max_ballast), 100));
  ok1(equals(fixed(polar.v1), 22.2222222));
  ok1(equals(fixed(polar.w1), -0.606));
  ok1(equals(fixed(polar.v2), 33.3333333));
  ok1(equals(fixed(polar.w2), -0.99));
  ok1(equals(fixed(polar.v3), 44.4444444));
  ok1(equals(fixed(polar.w3), -1.918));
  ok1(equals(fixed(polar.wing_area), 0.0));

  polar.ReadString(_T("318, 100, 80, -0.606, 120, -0.99, 160, -1.918, 9.8"));
  ok1(equals(fixed(polar.reference_mass), 318));
  ok1(equals(fixed(polar.max_ballast), 100));
  ok1(equals(fixed(polar.v1), 22.2222222));
  ok1(equals(fixed(polar.w1), -0.606));
  ok1(equals(fixed(polar.v2), 33.3333333));
  ok1(equals(fixed(polar.w2), -0.99));
  ok1(equals(fixed(polar.v3), 44.4444444));
  ok1(equals(fixed(polar.w3), -1.918));
  ok1(equals(fixed(polar.wing_area), 9.8));

  // Test GetString()
  TCHAR polar_string[255];
  polar.GetString(polar_string, 255);
  ok(_tcscmp(_T("318,100,80.000,-0.606,120.000,-0.990,160.000,-1.918,9.800"),
             polar_string) == 0, "GetString()");
}

static void
TestFileImport()
{
  // Test LoadFromFile()
  PolarInfo polar;
  PolarGlue::LoadFromFile(polar, _T("test/data/test.plr"));
  ok1(equals(fixed(polar.reference_mass), 318));
  ok1(equals(fixed(polar.max_ballast), 100));
  ok1(equals(fixed(polar.v1), 22.2222222));
  ok1(equals(fixed(polar.w1), -0.606));
  ok1(equals(fixed(polar.v2), 33.3333333));
  ok1(equals(fixed(polar.w2), -0.99));
  ok1(equals(fixed(polar.v3), 44.4444444));
  ok1(equals(fixed(polar.w3), -1.918));
  ok1(equals(fixed(polar.wing_area), 9.8));
}

static void
TestBuiltInPolars()
{
  unsigned count = PolarStore::Count();
  for(unsigned i = 0; i < count; i++) {
    PolarInfo polar = PolarStore::GetItem(i).ToPolarInfo();
#ifdef _UNICODE
  size_t wide_length = _tcslen(PolarStore::GetItem(i).name);
  char narrow[wide_length * 4 + 1];
  int narrow_length =
      ::WideCharToMultiByte(CP_ACP, 0, PolarStore::GetItem(i).name, wide_length,
                            narrow, sizeof(narrow), NULL, NULL);
  narrow[narrow_length] = 0;

  ok(polar.IsValid(), narrow);
#else
  ok(polar.IsValid(), PolarStore::GetItem(i).name);
#endif
  }
}

int main(int argc, char **argv)
{
  plan_tests(19 + 9 + PolarStore::Count());

  TestBasic();
  TestFileImport();
  TestBuiltInPolars();

  return exit_status();
}
