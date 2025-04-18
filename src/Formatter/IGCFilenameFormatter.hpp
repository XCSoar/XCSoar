// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct BrokenDate;

void FormatIGCFilename(TCHAR* buffer, const BrokenDate &date,
                       TCHAR manufacturer, const TCHAR *logger_id,
                       unsigned flight_number);
void FormatIGCFilenameLong(TCHAR* buffer, const BrokenDate &date,
                           const TCHAR *manufacturer, const TCHAR *logger_id,
                           unsigned flight_number);

#ifdef _UNICODE

void FormatIGCFilename(TCHAR* buffer, const BrokenDate &date,
                       char manufacturer, const char *logger_id,
                       unsigned flight_number);
void FormatIGCFilenameLong(TCHAR* buffer, const BrokenDate &date,
                           const char *manufacturer, const char *logger_id,
                           unsigned flight_number);

#endif
