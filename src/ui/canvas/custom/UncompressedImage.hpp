// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

#include <memory>

#include <cstdint>

class UncompressedImage {
public:
  enum class Format {
    INVALID,

    /**
     * 24 bits per pixel RGB.
     */
    RGB,

    /**
     * 24 bits per pixel RGB with 8 bit alpha.
     */
    RGBA,

    /**
     * 8 bits per pixel grayscale.
     */
    GRAY,
  };

private:
  Format format;

  /**
   * Flip up/down?  Some image formats (such as BMP and TIFF) store
   * the bottom-most row first.
   */
  bool flipped;

  std::size_t pitch;
  unsigned width, height;

  std::unique_ptr<uint8_t[]> data;

public:
  UncompressedImage() = default;

  UncompressedImage(Format _format, std::size_t _pitch,
                    unsigned _width, unsigned _height,
                    std::unique_ptr<uint8_t[]> &&_data,
                    bool _flipped=false) noexcept
    :format(_format), flipped(_flipped),
     pitch(_pitch), width(_width), height(_height),
     data(std::move(_data)) {}

  UncompressedImage(UncompressedImage &&other) = default;
  UncompressedImage(const UncompressedImage &other) = delete;

  UncompressedImage &operator=(UncompressedImage &&src) = default;
  UncompressedImage &operator=(const UncompressedImage &other) = delete;

  bool IsDefined() const {
    return !!data;
  }

  Format GetFormat() const {
    return format;
  }

  bool IsFlipped() const {
    return flipped;
  }

  std::size_t GetPitch() const noexcept {
    return pitch;
  }

  PixelSize GetSize() const {
    return {width, height};
  }

  unsigned GetWidth() const {
    return width;
  }

  unsigned GetHeight() const {
    return height;
  }

  const void *GetData() const {
    return data.get();
  }
};
