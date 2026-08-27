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

  /**
   * True if the icon contains meaningful colours (e.g. green/red
   * landable icons).  Dark-mode inversion is skipped for such icons
   * because inverting the colour channels would produce wrong colours.
   */
  bool has_colors = false;

public:
  const PixelSize &GetSize() const noexcept {
    return size;
  }

  /**
   * The size Draw(Canvas&, PixelPoint, unsigned) will paint for the
   * given target height.
   */
  [[gnu::pure]]
  PixelSize GetScaledSize(unsigned target_height) const noexcept {
    if (target_height == 0 || target_height == size.height ||
        size.height == 0)
      return size;

    return {size.width * target_height / size.height, target_height};
  }

  bool IsDefined() const noexcept {
    return bitmap.IsDefined();
  }

  void LoadResource(ResourceId id, ResourceId mdpi_id = ResourceId::Null(),
                    ResourceId xhdpi_id = ResourceId::Null(),
                    ResourceId xxhdpi_id = ResourceId::Null(),
                    bool center=true);

  void Reset() noexcept {
    bitmap.Reset();
  }

  void Draw(Canvas &canvas, PixelPoint p) const noexcept;

  /**
   * Draw the icon centred on @p p, uniformly scaled so its height
   * matches @p target_height.  If target_height is 0 the icon is
   * drawn at native size.
   */
  void Draw(Canvas &canvas, PixelPoint p,
            unsigned target_height) const noexcept;

  void Draw(Canvas &canvas, const PixelRect &rc, bool inverse) const noexcept;
};
