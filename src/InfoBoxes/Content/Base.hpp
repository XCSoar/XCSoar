// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
struct InfoBoxData;
struct InfoBoxPanel;
class Canvas;

class InfoBoxContent
{
public:
  enum InfoBoxKeyCodes {
    ibkLeft = -2,
    ibkDown = -1,
    ibkUp = 1,
    ibkRight = 2
  };

  virtual ~InfoBoxContent() noexcept;

  virtual void Update(InfoBoxData &data) noexcept = 0;
  virtual bool HandleKey(const InfoBoxKeyCodes keycode) noexcept;
  virtual bool HandleClick() noexcept;

  virtual void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept;

  [[gnu::pure]]
  virtual const InfoBoxPanel *GetDialogContent() noexcept;
};
