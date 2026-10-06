// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

struct FlarmRange;

/**
 * Parses one "$PFLAN,A,RANGE" sentence (radio range statistics) into
 * @p range.  Sentence types and channels this parser does not know
 * are ignored; the ICD may add new ones.
 *
 * @param fields the sentence after "PFLAN,A,RANGE," and before the
 * checksum, e.g. "RFTOP,A,5600,4800,,3600"
 */
void
ParsePFLANRange(std::string_view fields, FlarmRange &range);
