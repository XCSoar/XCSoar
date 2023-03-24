// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FlarmNameDatabase;
class TLineReader;
class BufferedOutputStream;

void
LoadFlarmNameFile(TLineReader &reader, FlarmNameDatabase &db);

void
SaveFlarmNameFile(BufferedOutputStream &writer, FlarmNameDatabase &db);
