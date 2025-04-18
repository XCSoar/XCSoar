// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"
#include "InfoBoxes/Content/Altitude.hpp"

class Validity;
class TraceVariableHistory;

class InfoBoxContentSpark: public InfoBoxContent
{
protected:
  void Paint(Canvas &canvas, const PixelRect &rc,
             const TraceVariableHistory &var,
             const bool center = true) noexcept;
  void SetVSpeedComment(InfoBoxData &data,
                        const TraceVariableHistory &var,
                        Validity validity) noexcept;
};

class InfoBoxContentVarioSpark : public InfoBoxContentSpark
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

class InfoBoxContentNettoVarioSpark : public InfoBoxContentSpark
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

class InfoBoxContentCirclingAverageSpark : public InfoBoxContentSpark
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

class InfoBoxContentBarogram : public InfoBoxContentAltitude
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
  const InfoBoxPanel *GetDialogContent() noexcept override;
};

class InfoBoxContentThermalBand : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};

class InfoBoxContentTaskProgress : public InfoBoxContent
{
public:
  void Update(InfoBoxData &data) noexcept override;
  void OnCustomPaint(Canvas &canvas, const PixelRect &rc) noexcept override;
};
