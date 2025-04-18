// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class GLTexture;
class UncompressedImage;

/**
 * Convert an #UncompressedImage to a #GLTexture.
 *
 * @return the new GLTexture object or nullptr on error
 */
GLTexture *
ImportTexture(const UncompressedImage &image);

/**
 * Convert an #UncompressedImage to a GL_ALPHA #GLTexture.
 *
 * @return the new GLTexture object or nullptr on error
 */
GLTexture *
ImportAlphaTexture(const UncompressedImage &image);
