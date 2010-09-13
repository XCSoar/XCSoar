/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Util/RadixTree.hpp"

#include <iostream>
#include <assert.h>

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
  rt.visit_all(sum);
  return sum.value;
}

static int
prefix_sum(const RadixTree<int> &rt, const TCHAR *prefix)
{
  Sum sum;
  rt.visit_prefix(prefix, sum);
  return sum.value;
}

template<typename T>
struct AscendingKeyVisitor {
  tstring last;

  void operator()(const TCHAR *key, const T &value) {
    assert(last.compare(key) <= 0);
    last = key;
  }
};

template<typename T>
static void
check_ascending_keys(const RadixTree<T> &tree)
{
  AscendingKeyVisitor<T> visitor;
  tree.visit_all_pairs(visitor);
}

int main(int argc, char **argv)
{
  TCHAR buffer[64], *suggest;

  RadixTree<int> irt;
  irt.add(_T("foo"), 42);
  assert(all_sum(irt) == 42);
  assert(prefix_sum(irt, _T("")) == 42);
  assert(prefix_sum(irt, _T("f")) == 42);
  assert(prefix_sum(irt, _T("fo")) == 42);
  assert(prefix_sum(irt, _T("foo")) == 42);
  assert(prefix_sum(irt, _T("foobar")) == 0);

  irt.add(_T("foa"), 0);
  assert(all_sum(irt) == 42);
  check_ascending_keys(irt);

  suggest = irt.suggest(_T("xyz"), buffer, 64);
  assert(suggest == NULL);

  suggest = irt.suggest(_T(""), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("f")) == 0);

  suggest = irt.suggest(_T("f"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("o")) == 0);

  suggest = irt.suggest(_T("foo"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("")) == 0);

  irt.add(_T("bar"), 1);
  assert(all_sum(irt) == 43);
  assert(prefix_sum(irt, _T("")) == 43);
  assert(prefix_sum(irt, _T("f")) == 42);

  suggest = irt.suggest(_T(""), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("bf")) == 0);

  suggest = irt.suggest(_T("ba"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("r")) == 0);

  irt.add(_T("foo"), 2);
  assert(all_sum(irt) == 45);
  assert(prefix_sum(irt, _T("")) == 45);
  assert(prefix_sum(irt, _T("f")) == 44);
  assert(prefix_sum(irt, _T("fo")) == 44);
  assert(prefix_sum(irt, _T("foo")) == 44);
  assert(prefix_sum(irt, _T("foobar")) == 0);

  suggest = irt.suggest(_T("foo"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("")) == 0);

  irt.add(_T("baz"), 3);
  assert(all_sum(irt) == 48);
  assert(prefix_sum(irt, _T("b")) == 4);
  assert(prefix_sum(irt, _T("ba")) == 4);
  assert(prefix_sum(irt, _T("bar")) == 1);
  assert(prefix_sum(irt, _T("baz")) == 3);

  suggest = irt.suggest(_T(""), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("bf")) == 0);

  suggest = irt.suggest(_T("ba"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("rz")) == 0);

  irt.add(_T("foobar"), 4);
  assert(all_sum(irt) == 52);
  assert(prefix_sum(irt, _T("f")) == 48);
  assert(prefix_sum(irt, _T("fo")) == 48);
  assert(prefix_sum(irt, _T("foo")) == 48);
  assert(prefix_sum(irt, _T("foobar")) == 4);

  suggest = irt.suggest(_T("foo"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("b")) == 0);

  irt.add(_T("fo"), 5);
  assert(all_sum(irt) == 57);
  assert(prefix_sum(irt, _T("f")) == 53);
  assert(prefix_sum(irt, _T("fo")) == 53);
  assert(prefix_sum(irt, _T("foo")) == 48);
  assert(prefix_sum(irt, _T("foobar")) == 4);

  irt.add(_T("fooz"), 6);
  assert(all_sum(irt) == 63);
  assert(prefix_sum(irt, _T("f")) == 59);
  assert(prefix_sum(irt, _T("fo")) == 59);
  assert(prefix_sum(irt, _T("foo")) == 54);

  suggest = irt.suggest(_T("foo"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("bz")) == 0);

  irt.add(_T("fooy"), 7);
  assert(all_sum(irt) == 70);
  assert(prefix_sum(irt, _T("f")) == 66);
  assert(prefix_sum(irt, _T("fo")) == 66);
  assert(prefix_sum(irt, _T("foo")) == 61);

  suggest = irt.suggest(_T("foo"), buffer, 64);
  assert(suggest != NULL);
  assert(_tcscmp(suggest, _T("byz")) == 0);

  irt.add(_T("foo"), 8);
  assert(all_sum(irt) == 78);
  assert(prefix_sum(irt, _T("foo")) == 69);

  irt.remove(_T("foo"), 42);
  assert(all_sum(irt) == 36);
  assert(prefix_sum(irt, _T("foo")) == 27);

  irt.remove(_T("foo"));
  assert(all_sum(irt) == 26);
  assert(prefix_sum(irt, _T("")) == 26);
  assert(prefix_sum(irt, _T("foo")) == 17);

  irt.add(_T(""), 9);
  assert(all_sum(irt) == 35);
  assert(prefix_sum(irt, _T("")) == 35);
  assert(prefix_sum(irt, _T("foo")) == 17);

  check_ascending_keys(irt);

  return 0;
}
