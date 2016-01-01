/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Util/StringAPI.hxx"
#include "Util/StringCompare.hxx"
#include "TestUtil.hpp"

#include <tchar.h>

int main(int argc, char **argv)
{
  plan_tests(29);

  // Test StringIsEqual()

  ok1(StringIsEqual("", ""));
  ok1(!StringIsEqual("a", ""));
  ok1(!StringIsEqual("", "a"));
  ok1(StringIsEqual("aaa", "aaa"));
  ok1(!StringIsEqual("aaa", "a"));
  ok1(!StringIsEqual("aaa", "bbb"));
  ok1(!StringIsEqual("bbb", "aaa"));

  ok1(StringIsEqual("This is a funny test case!",
                    "This is a funny test case!"));

  ok1(StringIsEqual(_T(""), _T("")));
  ok1(!StringIsEqual(_T("a"), _T("")));
  ok1(!StringIsEqual(_T(""), _T("a")));
  ok1(StringIsEqual(_T("aaa"), _T("aaa")));
  ok1(!StringIsEqual(_T("aaa"), _T("a")));
  ok1(!StringIsEqual(_T("aaa"), _T("bbb")));
  ok1(!StringIsEqual(_T("bbb"), _T("aaa")));

  ok1(StringIsEqual(_T("This is a funny test case!"),
                    _T("This is a funny test case!")));

  // Test StringStartsWith()

  ok1(StringStartsWith(_T(""), _T("")));
  ok1(StringStartsWith(_T("a"), _T("")));
  ok1(!StringStartsWith(_T(""), _T("a")));
  ok1(StringStartsWith(_T("aaa"), _T("aaa")));
  ok1(StringStartsWith(_T("aaa"), _T("a")));
  ok1(!StringStartsWith(_T("bbb"), _T("aaa")));

  // Test StringAfterPrefix()

  ok1(StringIsEqual(StringAfterPrefix(_T(""), _T("")), _T("")));
  ok1(StringIsEqual(StringAfterPrefix(_T("a"), _T("")), _T("a")));
  ok1(StringAfterPrefix(_T(""), _T("a")) == NULL);
  ok1(StringIsEqual(StringAfterPrefix(_T("aaa"), _T("aaa")), _T("")));
  ok1(StringIsEqual(StringAfterPrefix(_T("aaa"), _T("a")), _T("aa")));
  ok1(StringAfterPrefix(_T("bbb"), _T("aaa")) == NULL);
  ok1(StringIsEqual(StringAfterPrefix(_T("This is a funny test case!"),
                                      _T("This is")),
                                      _T(" a funny test case!")));

  return exit_status();
}
