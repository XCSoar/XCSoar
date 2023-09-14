// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BufferedLineReader.hpp"

char *
BufferedLineReader::ReadLine()
{
  return buffered.ReadLine();
}
