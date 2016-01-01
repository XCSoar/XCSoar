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

#define PRINT_RADIX_TREE

#include <iostream>

#include "Util/RadixTree.hpp"
#include "Util/StringAPI.hxx"
#include "TestUtil.hpp"

struct Sum {
  int value;

  Sum():value(0) {}

  void operator()(int x) {
    value += x;
  }
};

static int
all_sum(const RadixTree<int> &rt)
{
  Sum sum;
  rt.VisitAll(sum);
  return sum.value;
}

static int
prefix_sum(const RadixTree<int> &rt, const TCHAR *prefix)
{
  Sum sum;
  rt.VisitPrefix(prefix, sum);
  return sum.value;
}

template<typename T>
struct AscendingKeyVisitor
  : public std::binary_function<const TCHAR *, T, void> {
  tstring last;

  void operator()(const TCHAR *key, const T &value) {
    ok1(last.compare(key) <= 0);
    last = key;
  }
};

template<typename T>
static void
check_ascending_keys(const RadixTree<T> &tree)
{
  AscendingKeyVisitor<T> visitor;
  tree.VisitAllPairs(visitor);
}

int main(int argc, char **argv)
{
  plan_tests(86);

  TCHAR buffer[64], *suggest;

  RadixTree<int> irt;
  irt.Add(_T("foo"), 42);
  ok1(all_sum(irt) == 42);
  ok1(prefix_sum(irt, _T("")) == 42);
  ok1(prefix_sum(irt, _T("f")) == 42);
  ok1(prefix_sum(irt, _T("fo")) == 42);
  ok1(prefix_sum(irt, _T("foo")) == 42);
  ok1(prefix_sum(irt, _T("foobar")) == 0);

  irt.Add(_T("foa"), 0);
  ok1(all_sum(irt) == 42);
  check_ascending_keys(irt);

  suggest = irt.Suggest(_T("xyz"), buffer, 64);
  ok1(suggest == NULL);

  suggest = irt.Suggest(_T(""), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("f")));

  suggest = irt.Suggest(_T("f"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("o")));

  suggest = irt.Suggest(_T("foo"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("")));

  irt.Add(_T("bar"), 1);
  ok1(all_sum(irt) == 43);
  ok1(prefix_sum(irt, _T("")) == 43);
  ok1(prefix_sum(irt, _T("f")) == 42);

  suggest = irt.Suggest(_T(""), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("bf")));

  suggest = irt.Suggest(_T("ba"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("r")));

  irt.Add(_T("foo"), 2);
  ok1(all_sum(irt) == 45);
  ok1(prefix_sum(irt, _T("")) == 45);
  ok1(prefix_sum(irt, _T("f")) == 44);
  ok1(prefix_sum(irt, _T("fo")) == 44);
  ok1(prefix_sum(irt, _T("foo")) == 44);
  ok1(prefix_sum(irt, _T("foobar")) == 0);

  ok1(irt.Get(_T("foo"), 0) == 42 || irt.Get(_T("foo"), 0) == 2);
  ok1(irt.GetIf(_T("foo"), 0, std::bind1st(std::equal_to<int>(), 42)) == 42);
  ok1(irt.GetIf(_T("foo"), 0, std::bind1st(std::equal_to<int>(), 2)) == 2);
  ok1(irt.GetIf(_T("foo"), 0, std::bind1st(std::equal_to<int>(), 22)) == 0);

  suggest = irt.Suggest(_T("foo"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("")));

  irt.Add(_T("baz"), 3);
  ok1(all_sum(irt) == 48);
  ok1(prefix_sum(irt, _T("b")) == 4);
  ok1(prefix_sum(irt, _T("ba")) == 4);
  ok1(prefix_sum(irt, _T("bar")) == 1);
  ok1(prefix_sum(irt, _T("baz")) == 3);

  suggest = irt.Suggest(_T(""), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("bf")));

  suggest = irt.Suggest(_T("ba"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("rz")));

  irt.Add(_T("foobar"), 4);
  ok1(all_sum(irt) == 52);
  ok1(prefix_sum(irt, _T("f")) == 48);
  ok1(prefix_sum(irt, _T("fo")) == 48);
  ok1(prefix_sum(irt, _T("foo")) == 48);
  ok1(prefix_sum(irt, _T("foobar")) == 4);

  suggest = irt.Suggest(_T("foo"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("b")));

  irt.Add(_T("fo"), 5);
  ok1(all_sum(irt) == 57);
  ok1(prefix_sum(irt, _T("f")) == 53);
  ok1(prefix_sum(irt, _T("fo")) == 53);
  ok1(prefix_sum(irt, _T("foo")) == 48);
  ok1(prefix_sum(irt, _T("foobar")) == 4);

  irt.Add(_T("fooz"), 6);
  ok1(all_sum(irt) == 63);
  ok1(prefix_sum(irt, _T("f")) == 59);
  ok1(prefix_sum(irt, _T("fo")) == 59);
  ok1(prefix_sum(irt, _T("foo")) == 54);

  suggest = irt.Suggest(_T("foo"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("bz")));

  irt.Add(_T("fooy"), 7);
  ok1(all_sum(irt) == 70);
  ok1(prefix_sum(irt, _T("f")) == 66);
  ok1(prefix_sum(irt, _T("fo")) == 66);
  ok1(prefix_sum(irt, _T("foo")) == 61);

  suggest = irt.Suggest(_T("foo"), buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, _T("byz")));

  irt.Add(_T("foo"), 8);
  ok1(all_sum(irt) == 78);
  ok1(prefix_sum(irt, _T("foo")) == 69);

  irt.Remove(_T("foo"), 42);
  ok1(all_sum(irt) == 36);
  ok1(prefix_sum(irt, _T("foo")) == 27);

  irt.Remove(_T("foo"));
  ok1(all_sum(irt) == 26);
  ok1(prefix_sum(irt, _T("")) == 26);
  ok1(prefix_sum(irt, _T("foo")) == 17);

  irt.Add(_T(""), 9);
  ok1(all_sum(irt) == 35);
  ok1(prefix_sum(irt, _T("")) == 35);
  ok1(prefix_sum(irt, _T("foo")) == 17);

  check_ascending_keys(irt);

  return exit_status();
}
