// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct BrokenDate;

void FormatIGCFilename(char* buffer, const BrokenDate &date,
                       char manufacturer, const char *logger_id,
                       unsigned flight_number);
void FormatIGCFilenameLong(char* buffer, const BrokenDate &date,
                           const char *manufacturer, const char *logger_id,
                           unsigned flight_number);
