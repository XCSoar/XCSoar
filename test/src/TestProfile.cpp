// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Profile.hpp"
#include "io/FileLineReader.hpp"
#include "system/Path.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"
#include "util/StaticString.hxx"
#include "util/PrintException.hxx"

#include <stdlib.h>

static void
TestMap()
{
  Profile::Clear();

  {
    int value;
    ok1(!Profile::Exists("key1"));
    ok1(!Profile::Get("key1", value));
    Profile::Set("key1", 4);
    ok1(Profile::Exists("key1"));
    ok1(Profile::Get("key1", value));
    ok1(value == 4);
  }

  {
    short value;
    ok1(!Profile::Get("key2", value));
    Profile::Set("key2", 123);
    ok1(Profile::Get("key2", value));
    ok1(value == 123);
  }

  {
    unsigned value;
    ok1(!Profile::Get("key3", value));
    Profile::Set("key3", -42);
    ok1(Profile::Get("key3", value));
    ok1(value == -42u);
  }

  {
    bool value;
    ok1(!Profile::Get("key4", value));
    Profile::Set("key4", true);
    ok1(Profile::Get("key4", value));
    ok1(value);
    Profile::Set("key4", false);
    ok1(Profile::Get("key4", value));
    ok1(!value);
  }

  {
    double value;
    ok1(!Profile::Get("key5", value));
    Profile::Set("key5", 1.337);
    ok1(Profile::Get("key5", value));
    ok1(equals(value, 1.337));
  }
}

static void
TestWriter()
{
  Profile::Clear();
  Profile::Set("key1", 4);
  Profile::Set("key2", "value2");

  Profile::SaveFile(Path(_T("output/TestProfileWriter.prf")));

  FileLineReaderA reader(Path(_T("output/TestProfileWriter.prf")));

  unsigned count = 0;
  bool found1 = false, found2 = false;

  char *line;
  while ((line = reader.ReadLine()) != NULL) {
    if (StringIsEqual(line, "key1=\"4\""))
      found1 = true;
    if (StringIsEqual(line, "key2=\"value2\""))
      found2 = true;

    count++;
  }

  ok1(count == 2);
  ok1(found1);
  ok1(found2);
}

static void
TestReader()
{
  Profile::Clear();
  Profile::LoadFile(Path(_T("test/data/TestProfileReader.prf")));

  {
    int value;
    ok1(Profile::Exists("key1"));
    ok1(Profile::Get("key1", value));
    ok1(value == 1);
  }

  {
    StaticString<32> value;
    ok1(Profile::Exists("key2"));
    ok1(Profile::Get("key2", value));
    ok1(value == _T("value"));
  }

  {
    int value;
    ok1(Profile::Exists("key3"));
    ok1(Profile::Get("key3", value));
    ok1(value == 5);
  }
}

int main()
try {
  plan_tests(31);

  TestMap();
  TestWriter();
  TestReader();

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
