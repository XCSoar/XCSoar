// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;

namespace OpenGL {

/**
 * Map the specified rectangle from Canvas to viewport coordinates.
 */
void ToViewport(PixelRect &rc);

} // namespace OpenGL
