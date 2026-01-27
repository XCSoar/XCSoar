// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct UnsignedPoint2D;
enum class DisplayOrientation : uint8_t;

namespace OpenGL {

/**
 * Initialize our OpenGL library.  Call when XCSoar starts.
 *
 * Throws on error.
 */
void Initialise();

/**
 * Set up our OpenGL library.  Call after the video mode and the
 * OpenGL context have been set up.
 *
 * Throws on error.
 */
void SetupContext();

/**
 * Set up the viewport and the matrices for 2D drawing.  Apply the
 * #DisplayOrientation via glRotatef() (OpenGL projection matrix).
 *
 * @param size the native (physical) size in pixels
 * @param content_size when non-zero, the logical size for the
 * projection (HiDPI: map logical coords to physical viewport)
 * @return the logical screen size (content_size if set, else size)
 */
UnsignedPoint2D SetupViewport(UnsignedPoint2D size) noexcept;

UnsignedPoint2D SetupViewport(UnsignedPoint2D size,
                              UnsignedPoint2D content_size) noexcept;

/**
 * Deinitialize our OpenGL library.  Call before shutdown.
 */
void Deinitialise() noexcept;

} // namespace OpenGL
