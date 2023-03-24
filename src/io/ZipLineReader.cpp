// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ZipLineReader.hpp"

long
ZipLineReaderA::GetSize() const
{
  return zip.GetSize();
}

long
ZipLineReaderA::Tell() const
{
  return zip.GetPosition();
}
