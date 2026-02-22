// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

class Path;
class UncompressedImage;

[[nodiscard]]
UncompressedImage
LoadJPEGFile(Path path);

[[nodiscard]]
UncompressedImage
LoadJPEG(std::span<const std::byte> buffer);
