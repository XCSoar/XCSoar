// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Polar/PolarFileGlue.hpp"
#include "Parser.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"

bool
PolarGlue::LoadFromFile(PolarInfo &polar, NLineReader &reader)
{
  const char *line;
  while ((line = reader.ReadLine()) != nullptr)
    if (ParsePolar(polar, line))
      return true;

  return false;
}

void
PolarGlue::LoadFromFile(PolarInfo &polar, Path path)
{
  FileLineReaderA reader(path);
  LoadFromFile(polar, reader);
}

void
PolarGlue::SaveToFile(const PolarInfo &polar, BufferedOutputStream &writer)
{
  char buffer[256];
  FormatPolar(polar, buffer, 256);
  writer.Write(buffer);
  writer.Write('\n');
}

void
PolarGlue::SaveToFile(const PolarInfo &polar, Path path)
{
  FileOutputStream file(path);
  BufferedOutputStream writer(file);
  SaveToFile(polar, writer);
  writer.Flush();
  file.Commit();
}
