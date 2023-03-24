// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstddef>

/**
 * Split line (comma separated fields) in individual fields.
 * @param src The source line of comma separated fields
 * @param dst Destination buffer containing processed '\0' separated fields.
 * @param arr Array of pointers pointing to individual fields of dst
 * @param trim Optional flag to request space character removal at beginning
 * and end of fields.
 * @param quote_char Optional character used for quoting of individual fields.
 * Allows handling of quoted strings (e.g. fields containing leading or
 * trailing space or "," characters). Only considers the quote character
 * at the beginning or end of fields e.g. 6°10'22"E would be returned as is.
 * @return number of fields returned. Note: an empty src returns 1 for
 * for consistency (i.e. "" -> 1, "," -> 2)
 */
size_t
ExtractParameters(const TCHAR *src, TCHAR *dst,
                  const TCHAR **arr, size_t sz,
                  const bool trim=false,
                  const TCHAR quote_char=_T('\0'));
