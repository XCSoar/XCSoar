// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PolarInfo;
class Path;
class NLineReader;
class BufferedOutputStream;
class Error;

namespace PolarGlue
{
  void LoadFromFile(PolarInfo &polar, Path path);
  void SaveToFile(const PolarInfo &polar, Path path);
  bool LoadFromFile(PolarInfo &polar, NLineReader &reader);
  void SaveToFile(const PolarInfo &polar, BufferedOutputStream &writer);
}
