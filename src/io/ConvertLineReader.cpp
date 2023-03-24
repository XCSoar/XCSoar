// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConvertLineReader.hpp"

ConvertLineReader::ConvertLineReader(std::unique_ptr<LineReader<char>> &&_source,
                                     Charset cs)
  :source(std::move(_source)),
   converter(cs)
{
}

TCHAR *
ConvertLineReader::ReadLine()
{
  char *narrow = source->ReadLine();

  if (narrow == nullptr)
    return nullptr;

  return converter.Convert(narrow);
}

long
ConvertLineReader::GetSize() const
{
  return source->GetSize();
}

long
ConvertLineReader::Tell() const
{
  return source->Tell();
}
