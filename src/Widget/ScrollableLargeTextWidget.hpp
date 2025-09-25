// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Look/DialogLook.hpp"
#include "Widget/LargeTextWidget.hpp"

class ScrollableLargeTextWidget final : public LargeTextWidget {
  const DialogLook &look;
  std::basic_string<TCHAR> text;

public:
  ScrollableLargeTextWidget(const DialogLook &dlgLook, const TCHAR *t);

  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
};
