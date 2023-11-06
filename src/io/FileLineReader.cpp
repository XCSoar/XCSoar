// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileLineReader.hpp"

char *
FileLineReaderA::ReadLine()
{
  return buffered.ReadLine();
}
