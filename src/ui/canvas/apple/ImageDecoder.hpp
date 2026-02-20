// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

class Path;
class UncompressedImage;

UncompressedImage
LoadJPEGFile(Path path) noexcept;

UncompressedImage
LoadJPEG(std::span<const std::byte> buffer);

UncompressedImage
LoadPNG(std::span<const std::byte> buffer);

UncompressedImage
LoadPNG(Path path) noexcept;
