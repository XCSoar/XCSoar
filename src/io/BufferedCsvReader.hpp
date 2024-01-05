// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/BufferedReader.hxx"

#include <span>
#include <optional>
#include <string_view>

/**
 * Read one record from the CSV file and fills `columns` with pointers to the (unquoted) values of each column.
 *
 * @return the number of columns
 *
 * @see RFC 4180
 */

size_t
ReadCsvRecord(BufferedReader &reader, std::span<std::string_view> columns);
