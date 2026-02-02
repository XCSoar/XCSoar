// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FlarmMessagingDatabase;
class BufferedReader;
class BufferedOutputStream;

unsigned
LoadFlarmMessagingFile(BufferedReader &reader, FlarmMessagingDatabase &db);

void
SaveFlarmMessagingFile(BufferedOutputStream &writer, FlarmMessagingDatabase &db);
