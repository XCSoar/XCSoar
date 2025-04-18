// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct KeyValuePair
{
  char *key;
  char *value;
};

class NLineReader;

class KeyValueFileReader
{
protected:
  NLineReader &reader;

public:
  KeyValueFileReader(NLineReader &_reader):reader(_reader) {}
  bool Read(KeyValuePair &pair);
};
