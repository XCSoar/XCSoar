// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NameDatabase.hpp"

int
FlarmNameDatabase::Find(FlarmId id) const noexcept
{
  assert(id.IsDefined());

  for (unsigned i = 0, size = data.size(); i != size; ++i)
    if (data[i].id == id)
      return i;

  return -1;
}

int
FlarmNameDatabase::Find(const char *name) const noexcept
{
  assert(name != nullptr);

  for (unsigned i = 0, size = data.size(); i != size; ++i)
    if (data[i].name.equals(name))
      return i;

  return -1;
}

const char *
FlarmNameDatabase::Get(FlarmId id) const noexcept
{
  int i = Find(id);
  if (i < 0)
    return nullptr;

  return data[i].name;
}

FlarmId
FlarmNameDatabase::Get(const char *name) const noexcept
{
  int i = Find(name);
  if (i < 0)
    return FlarmId::Undefined();

  return data[i].id;
}

unsigned
FlarmNameDatabase::Get(const char *name,
                       FlarmId *buffer, unsigned max) const noexcept
{
  assert(name != nullptr);
  assert(buffer != nullptr);
  assert(max > 0);

  unsigned n = 0;
  for (unsigned i = 0, size = data.size(); i != size && n != max; ++i)
    if (data[i].name.equals(name))
      buffer[n++] = data[i].id;

  return n;
}

bool
FlarmNameDatabase::Set(FlarmId id, const char *name) noexcept
{
  assert(id.IsDefined());
  assert(name != nullptr);

  int i = Find(id);
  if (i >= 0) {
    if (!StringIsEqual(name, _T("")))
      /* update existing record */
      data[i].name = name;
    else
      /* remove record if empty */
      Remove(id);

    return true;
  } else if (!data.full()) {
    /* create new record */
    data.append({id, name});
    return true;
  } else
    /* error: database is full */
    return false;
}

bool
FlarmNameDatabase::Remove(FlarmId id) noexcept
{
  assert(id.IsDefined());

  int i = Find(id);
  if (i >= 0) {
    data.remove(i);
    return true;
  }

  return false;
}
