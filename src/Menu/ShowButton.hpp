// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Button.hpp"

struct ButtonLook;

#ifdef ANDROID
#include "ui/canvas/Bitmap.hpp"
#endif

/* map overlay menu button (hamburger icon) */
class ShowMenuButton : public Button {
public:
  void Create(ContainerWindow &parent, const ButtonLook &look,
              const PixelRect &rc,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};

/* map overlay zoom button (+ or -) */
class ShowZoomButton : public Button {
public:
  enum class Sign {
    ZOOM_OUT,
    ZOOM_IN,
  };

  void Create(ContainerWindow &parent, const ButtonLook &look,
              const PixelRect &rc, Sign sign,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;

private:
  Sign sign;
};

#ifdef ANDROID
/* rotate screen button */
class ShowRotateButton : public Button {
  Bitmap bitmap;

public:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};
#endif
