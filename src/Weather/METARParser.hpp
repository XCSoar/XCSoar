// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct METAR;
struct ParsedMETAR;

namespace METARParser {

bool
Parse(const METAR &metar, ParsedMETAR &parsed);

}
