// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define PRINT_RADIX_TREE

#include <iostream>

#include "util/RadixTree.hpp"
#include "util/StringAPI.hxx"
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
prefix_sum(const RadixTree<int> &rt, const char *prefix)
{
  Sum sum;
  rt.VisitPrefix(prefix, sum);
  return sum.value;
}

template<typename T>
struct AscendingKeyVisitor {
  std::string last;

  void operator()(const char *key, [[maybe_unused]] const T &value) {
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

int main()
{
  plan_tests(86);

  char buffer[64], *suggest;

  RadixTree<int> irt;
  irt.Add("foo", 42);
  ok1(all_sum(irt) == 42);
  ok1(prefix_sum(irt, "") == 42);
  ok1(prefix_sum(irt, "f") == 42);
  ok1(prefix_sum(irt, "fo") == 42);
  ok1(prefix_sum(irt, "foo") == 42);
  ok1(prefix_sum(irt, "foobar") == 0);

  irt.Add("foa", 0);
  ok1(all_sum(irt) == 42);
  check_ascending_keys(irt);

  suggest = irt.Suggest("xyz", buffer, 64);
  ok1(suggest == NULL);

  suggest = irt.Suggest("", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "f"));

  suggest = irt.Suggest("f", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "o"));

  suggest = irt.Suggest("foo", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, ""));

  irt.Add("bar", 1);
  ok1(all_sum(irt) == 43);
  ok1(prefix_sum(irt, "") == 43);
  ok1(prefix_sum(irt, "f") == 42);

  suggest = irt.Suggest("", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "bf"));

  suggest = irt.Suggest("ba", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "r"));

  irt.Add("foo", 2);
  ok1(all_sum(irt) == 45);
  ok1(prefix_sum(irt, "") == 45);
  ok1(prefix_sum(irt, "f") == 44);
  ok1(prefix_sum(irt, "fo") == 44);
  ok1(prefix_sum(irt, "foo") == 44);
  ok1(prefix_sum(irt, "foobar") == 0);

  ok1(irt.Get("foo", 0) == 42 || irt.Get("foo", 0) == 2);
  ok1(irt.GetIf("foo", 0, [](auto i){ return i == 42; }) == 42);
  ok1(irt.GetIf("foo", 0, [](auto i){ return i == 2; }) == 2);
  ok1(irt.GetIf("foo", 0, [](auto i){ return i == 22; }) == 0);

  suggest = irt.Suggest("foo", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, ""));

  irt.Add("baz", 3);
  ok1(all_sum(irt) == 48);
  ok1(prefix_sum(irt, "b") == 4);
  ok1(prefix_sum(irt, "ba") == 4);
  ok1(prefix_sum(irt, "bar") == 1);
  ok1(prefix_sum(irt, "baz") == 3);

  suggest = irt.Suggest("", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "bf"));

  suggest = irt.Suggest("ba", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "rz"));

  irt.Add("foobar", 4);
  ok1(all_sum(irt) == 52);
  ok1(prefix_sum(irt, "f") == 48);
  ok1(prefix_sum(irt, "fo") == 48);
  ok1(prefix_sum(irt, "foo") == 48);
  ok1(prefix_sum(irt, "foobar") == 4);

  suggest = irt.Suggest("foo", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "b"));

  irt.Add("fo", 5);
  ok1(all_sum(irt) == 57);
  ok1(prefix_sum(irt, "f") == 53);
  ok1(prefix_sum(irt, "fo") == 53);
  ok1(prefix_sum(irt, "foo") == 48);
  ok1(prefix_sum(irt, "foobar") == 4);

  irt.Add("fooz", 6);
  ok1(all_sum(irt) == 63);
  ok1(prefix_sum(irt, "f") == 59);
  ok1(prefix_sum(irt, "fo") == 59);
  ok1(prefix_sum(irt, "foo") == 54);

  suggest = irt.Suggest("foo", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "bz"));

  irt.Add("fooy", 7);
  ok1(all_sum(irt) == 70);
  ok1(prefix_sum(irt, "f") == 66);
  ok1(prefix_sum(irt, "fo") == 66);
  ok1(prefix_sum(irt, "foo") == 61);

  suggest = irt.Suggest("foo", buffer, 64);
  ok1(suggest != NULL);
  ok1(StringIsEqual(suggest, "byz"));

  irt.Add("foo", 8);
  ok1(all_sum(irt) == 78);
  ok1(prefix_sum(irt, "foo") == 69);

  irt.Remove("foo", 42);
  ok1(all_sum(irt) == 36);
  ok1(prefix_sum(irt, "foo") == 27);

  irt.Remove("foo");
  ok1(all_sum(irt) == 26);
  ok1(prefix_sum(irt, "") == 26);
  ok1(prefix_sum(irt, "foo") == 17);

  irt.Add("", 9);
  ok1(all_sum(irt) == 35);
  ok1(prefix_sum(irt, "") == 35);
  ok1(prefix_sum(irt, "foo") == 17);

  check_ascending_keys(irt);

  return exit_status();
}
