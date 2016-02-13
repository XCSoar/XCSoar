/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_UNCOMPRESSED_IMAGE_HPP
#define XCSOAR_UNCOMPRESSED_IMAGE_HPP

#include "Screen/Point.hpp"

#include <memory>

#include <stdint.h>

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

  unsigned pitch, width, height;

  std::unique_ptr<uint8_t[]> data;

public:
  UncompressedImage() = default;

  UncompressedImage(Format _format, unsigned _pitch,
                    unsigned _width, unsigned _height,
                    std::unique_ptr<uint8_t[]> &&_data,
                    bool _flipped=false)
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

  unsigned GetPitch() const {
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

#endif
