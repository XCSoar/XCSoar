// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/* Kept for compatibility with older commits on this branch.
   New code should include MapWindow/Overlays/MbTilesDatabase.hpp. */
#include "MapWindow/Overlays/MbTilesDatabase.hpp"

namespace EDL {
using MbTilesMetadata = MbTiles::Metadata;
using TileKey = MbTiles::TileKey;
using MbTilesDatabase = MbTiles::Database;
} // namespace EDL
