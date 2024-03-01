// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Bitmap.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"
#include "ResourceId.hpp"

struct PixelRect;
class Canvas;

/**
 * An icon with a mask which marks transparent pixels.
 */
class MaskedIcon {
protected:
  Bitmap bitmap;

  PixelSize size;

  PixelPoint origin;

public:
  const PixelSize &GetSize() const noexcept {
    return size;
  }

  bool IsDefined() const noexcept {
    return bitmap.IsDefined();
  }

  void LoadResource(ResourceId id, ResourceId big_id = ResourceId::Null(),
                    ResourceId ultra_id = ResourceId::Null(),
                    bool center=true);

  void Reset() noexcept {
    bitmap.Reset();
  }

  void Draw(Canvas &canvas, PixelPoint p) const noexcept;

  void Draw(Canvas &canvas, const PixelRect &rc, bool inverse) const noexcept;
};
