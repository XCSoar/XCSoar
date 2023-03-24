// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

namespace LiveTrack24 {

typedef uint32_t UserID;
typedef uint32_t SessionID;

/** Generates a random session id */
SessionID
GenerateSessionID() noexcept;

/** Generates a random session id containing the given user id */
SessionID
GenerateSessionID(UserID user_id) noexcept;

} // namespace Livetrack24
