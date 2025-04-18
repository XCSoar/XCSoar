// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

class UncompressedImage;
class Path;

/**
 * Throws on error.
 */
UncompressedImage
LoadPNG(std::span<const std::byte> raw);

/**
 * Throws on error.
 */
UncompressedImage
LoadPNG(Path path);
