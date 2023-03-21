// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
struct Plane;
class KeyValueFileReader;
class KeyValueFileWriter;

namespace PlaneGlue
{
  bool Read(Plane &plane, KeyValueFileReader &reader);
  bool ReadFile(Plane &plane, Path path);
  void Write(const Plane &plane, KeyValueFileWriter &writer);

/**
 * Throws exception on error.
 */
void WriteFile(const Plane &plane, Path path);
}
