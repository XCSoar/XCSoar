// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Resources.hpp"
#include "Widget/WindowWidget.hpp"

class Canvas;

class GestureHelpWindow final : public PaintWindow {
  Bitmap down_img{IDB_GESTURE_DOWN}, dl_img{IDB_GESTURE_DL}, dr_img{IDB_GESTURE_DR},
         du_img{IDB_GESTURE_DU}, left_img{IDB_GESTURE_LEFT}, ldr_img{IDB_GESTURE_LDR},
         ldrdl_img{IDB_GESTURE_LDRDL}, right_img{IDB_GESTURE_RIGHT}, rd_img{IDB_GESTURE_RD},
         rl_img{IDB_GESTURE_RL}, up_img{IDB_GESTURE_UP}, ud_img{IDB_GESTURE_UD},
         uldr_img{IDB_GESTURE_ULDR}, urd_img{IDB_GESTURE_URD}, urdl_img{IDB_GESTURE_URDL};

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
