// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/BoundedArray.hxx"
#include "TestUtil.hpp"

struct Entry {
  unsigned key;
  unsigned age;
};

static constexpr auto GET_KEY = [](const Entry &entry) noexcept {
  return entry.key;
};

static constexpr auto GET_AGE = [](const Entry &entry) noexcept {
  return entry.age;
};

int
main()
{
  plan_tests(17);

  TrivialArray<Entry, 3> entries;
  entries.clear();

  ok1(BoundedArray::FindByKey(entries, 1u, GET_KEY) == nullptr);

  auto first = BoundedArray::AppendOrReplaceOldest(entries, GET_AGE);
  ok1(!first.replaced);
  first.value = {1, 20};
  ok1(entries.size() == 1);
  ok1(BoundedArray::FindByKey(entries, 1u, GET_KEY) == &first.value);

  auto second = BoundedArray::AppendOrReplaceOldest(entries, GET_AGE);
  ok1(!second.replaced);
  second.value = {2, 10};
  ok1(entries.size() == 2);

  auto third = BoundedArray::AppendOrReplaceOldest(entries, GET_AGE);
  ok1(!third.replaced);
  third.value = {3, 30};
  ok1(entries.full());

  auto replacement = BoundedArray::AppendOrReplaceOldest(entries, GET_AGE);
  ok1(replacement.replaced);
  ok1(&replacement.value == &second.value);
  replacement.value = {4, 40};
  ok1(entries.size() == 3);
  ok1(BoundedArray::FindByKey(entries, 2u, GET_KEY) == nullptr);
  ok1(BoundedArray::FindByKey(entries, 4u, GET_KEY) == &replacement.value);

  entries[0].age = 5;
  entries[1].age = 5;
  entries[2].age = 10;
  auto tied = BoundedArray::AppendOrReplaceOldest(entries, GET_AGE);
  ok1(tied.replaced);
  ok1(&tied.value == &entries[0]);

  const auto &const_entries = entries;
  ok1(BoundedArray::FindByKey(const_entries, 4u, GET_KEY) == &entries[1]);
  ok1(BoundedArray::FindByKey(const_entries, 99u, GET_KEY) == nullptr);

  return exit_status();
}
