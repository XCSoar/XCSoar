// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/BoundedArray.hxx"

extern "C" {
#include "tap.h"
}

struct Entry {
  unsigned key;
  unsigned age;
};

static constexpr auto GetKey = [](const Entry &entry) noexcept {
  return entry.key;
};

static constexpr auto GetAge = [](const Entry &entry) noexcept {
  return entry.age;
};

int
main()
{
  plan_tests(17);

  TrivialArray<Entry, 3> entries;
  entries.clear();

  ok1(BoundedArray::FindByKey(entries, 1u, GetKey) == nullptr);

  auto first = BoundedArray::AppendOrReplaceOldest(entries, GetAge);
  ok1(!first.replaced);
  first.value = {1, 20};
  ok1(entries.size() == 1);
  ok1(BoundedArray::FindByKey(entries, 1u, GetKey) == &first.value);

  auto second = BoundedArray::AppendOrReplaceOldest(entries, GetAge);
  ok1(!second.replaced);
  second.value = {2, 10};
  ok1(entries.size() == 2);

  auto third = BoundedArray::AppendOrReplaceOldest(entries, GetAge);
  ok1(!third.replaced);
  third.value = {3, 30};
  ok1(entries.full());

  auto replacement = BoundedArray::AppendOrReplaceOldest(entries, GetAge);
  ok1(replacement.replaced);
  ok1(&replacement.value == &second.value);
  replacement.value = {4, 40};
  ok1(entries.size() == 3);
  ok1(BoundedArray::FindByKey(entries, 2u, GetKey) == nullptr);
  ok1(BoundedArray::FindByKey(entries, 4u, GetKey) == &replacement.value);

  entries[0].age = 5;
  entries[1].age = 5;
  entries[2].age = 10;
  auto tied = BoundedArray::AppendOrReplaceOldest(entries, GetAge);
  ok1(tied.replaced);
  ok1(&tied.value == &entries[0]);

  const auto &const_entries = entries;
  ok1(BoundedArray::FindByKey(const_entries, 4u, GetKey) == &entries[1]);
  ok1(BoundedArray::FindByKey(const_entries, 99u, GetKey) == nullptr);

  return exit_status();
}
