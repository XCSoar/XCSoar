// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

class Bitmap;

/**
 * A widget which displays an image (a #Bitmap instance which is
 * managed by the caller).
 */
class ViewImageWidget : public WindowWidget {
  const Bitmap *bitmap;
  int zoom = 0;

public:
  explicit ViewImageWidget(const Bitmap *_bitmap=nullptr) noexcept
    :bitmap(_bitmap) {}

  explicit ViewImageWidget(const Bitmap &_bitmap) noexcept
    :bitmap(&_bitmap) {}

  void SetBitmap(const Bitmap *_bitmap) noexcept;

  void SetBitmap(const Bitmap &_bitmap) noexcept {
    SetBitmap(&_bitmap);
  }

protected:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
