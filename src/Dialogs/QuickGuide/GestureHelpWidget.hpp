// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Resources.hpp"
#include "Widget/WindowWidget.hpp"

class Canvas;

class GestureHelpWindow final : public PaintWindow {
  Bitmap down_img{IDB_GESTURE_DOWN};
  Bitmap dl_img{IDB_GESTURE_DL};
  Bitmap dr_img{IDB_GESTURE_DR};
  Bitmap du_img{IDB_GESTURE_DU};
  Bitmap left_img{IDB_GESTURE_LEFT};
  Bitmap ldr_img{IDB_GESTURE_LDR};
  Bitmap ldrdl_img{IDB_GESTURE_LDRDL};
  Bitmap right_img{IDB_GESTURE_RIGHT};
  Bitmap rd_img{IDB_GESTURE_RD};
  Bitmap rl_img{IDB_GESTURE_RL};
  Bitmap up_img{IDB_GESTURE_UP};
  Bitmap ud_img{IDB_GESTURE_UD};
  Bitmap uldr_img{IDB_GESTURE_ULDR};
  Bitmap urd_img{IDB_GESTURE_URD};
  Bitmap urdl_img{IDB_GESTURE_URDL};

public:
  static unsigned Layout(Canvas *canvas, const PixelRect &rc,
                         GestureHelpWindow *window) noexcept;

protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

class GestureHelpWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
