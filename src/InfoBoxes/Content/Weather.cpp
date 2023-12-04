// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Weather.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/WindEdit.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Screen/Layout.hpp"
#include "ui/dim/Rect.hpp"
#include "Renderer/RadarRenderer.hpp"
#include "Renderer/WindArrowRenderer.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#include <tchar.h>

void
UpdateInfoBoxHumidity(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.humidity_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.FmtValue( _T("{}"), (int)basic.humidity);
}

void
UpdateInfoBoxTemperature(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.temperature_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.FmtValue(_T("{:2.1f}"), basic.temperature.ToUser());

  data.SetValueUnit(Units::current.temperature_unit);
}

void
InfoBoxContentTemperatureForecast::Update(InfoBoxData &data) noexcept
{
  auto temperature = CommonInterface::GetComputerSettings().forecast_temperature;
  data.FmtValue(_T("{:2.1f}"), temperature.ToUser());

  data.SetValueUnit(Units::current.temperature_unit);
}

bool
InfoBoxContentTemperatureForecast::HandleKey(const InfoBoxKeyCodes keycode) noexcept
{
  switch(keycode) {
  case ibkUp:
    CommonInterface::SetComputerSettings().forecast_temperature += Temperature::FromKelvin(0.5);
    return true;

  case ibkDown:
    CommonInterface::SetComputerSettings().forecast_temperature -= Temperature::FromKelvin(0.5);
    return true;

  default:
    break;
  }

  return false;
}

/*
 * Subpart callback function pointers
 */

#ifdef __clang__
/* gcc gives "redeclaration differs in 'constexpr'" */
constexpr
#endif
const InfoBoxPanel wind_infobox_panels[] = {
  { N_("Edit"), LoadWindEditPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentWindArrow::GetDialogContent() noexcept
{
  return wind_infobox_panels;
}

void
UpdateInfoBoxWindSpeed(InfoBoxData &data) noexcept
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.FmtValue(_T("{:2.0f}"), Units::ToUserWindSpeed(info.wind.norm));

  // Set Unit
  data.SetValueUnit(Units::current.wind_speed_unit);

  // Set Comment
  data.SetComment(info.wind.bearing);
}

void
UpdateInfoBoxWindBearing(InfoBoxData &data) noexcept
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available) {
    data.SetInvalid();
    return;
  }

  data.SetValue(info.wind.bearing);

  TCHAR buffer[16];
  FormatUserWindSpeed(info.wind.norm, buffer, true, false);
  data.SetComment(buffer);
}

void UpdateInfoBoxInstWindSpeed(InfoBoxData &data) noexcept {
  const auto &info = CommonInterface::Basic();
  if (!info.external_instantaneous_wind_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.FmtValue(_T("{:2.0f}"),
                Units::ToUserWindSpeed(info.external_instantaneous_wind.norm));

  // Set Unit
  data.SetValueUnit(Units::current.wind_speed_unit);

  // Set Comment
  data.SetComment(info.external_instantaneous_wind.bearing);
}

void UpdateInfoBoxInstWindBearing(InfoBoxData &data) noexcept
{
  const auto &info = CommonInterface::Basic();
  if (!info.external_instantaneous_wind_available) {
    data.SetInvalid();
    return;
  }

  data.SetValue(info.external_instantaneous_wind.bearing);

  TCHAR buffer[16];
  FormatUserWindSpeed(info.external_instantaneous_wind.norm,
                      buffer, true, false);
  data.SetComment(buffer);
}

void
UpdateInfoBoxHeadWind(InfoBoxData &data) noexcept
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.head_wind_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.FmtValue(_T("{:2.0f}"), Units::ToUserWindSpeed(info.head_wind));

  // Set Unit
  data.SetValueUnit(Units::current.wind_speed_unit);
}

void
UpdateInfoBoxHeadWindSimplified(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.ground_speed_available || !basic.airspeed_available) {
    data.SetInvalid();
    return;
  }

  auto value = basic.true_airspeed - basic.ground_speed;

  // Set Value
  data.FmtValue(_T("{:2.0f}"), Units::ToUserWindSpeed(value));

  // Set Unit
  data.SetValueUnit(Units::current.wind_speed_unit);
}

void
InfoBoxContentWindArrow::Update(InfoBoxData &data) noexcept
{
  const DerivedInfo &info = CommonInterface::Calculated();
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!info.wind_available || info.wind.IsZero()) {
    data.SetInvalid();
    return;
  }

  data.SetCustom(info.wind_available.ToInteger() +
                 basic.attitude.heading_available.ToInteger());

  TCHAR speed_buffer[16];
  FormatUserWindSpeed(info.wind.norm, speed_buffer, true, false);

  StaticString<36> buffer;
  buffer.Format(_T("%s / %s"),
                FormatBearing(info.wind.bearing).c_str(),
                speed_buffer);
  data.SetComment(buffer);
}

void
PaintWindArrow(Canvas &canvas, const PixelRect &rc,
               const SpeedVector &wind, const Brush &brush) noexcept
{
  constexpr unsigned arrow_width = 6;
  constexpr unsigned arrow_tail_length = 3;

  const unsigned scale = Layout::Scale(100U);

  RadarRenderer radar_renderer{Layout::FastScale(10u)};
  radar_renderer.UpdateLayout(rc);

  // Normalize the size because the Layout::Scale is applied
  // by the DrawArrow() function again
  const unsigned size = radar_renderer.GetRadius() * 100 / scale;

  auto angle = wind.bearing - CommonInterface::Basic().attitude.heading;

  const int length =
    std::min(size, std::max(10u, uround(4 * wind.norm)));

  const int offset = -length / 2;

  const auto style = CommonInterface::GetMapSettings().wind_arrow_style;

  WindArrowRenderer renderer(UIGlobals::GetLook().wind_arrow_info_box);
  renderer.DrawArrow(canvas, radar_renderer.GetCenter(), angle,
                     arrow_width, length, arrow_tail_length,
                     style, offset, scale, brush);
}

void
InfoBoxContentWindArrow::OnCustomPaint(Canvas &canvas,
                                       const PixelRect &rc) noexcept
{
 
  const auto &info = CommonInterface::Calculated();
  const auto &basic = CommonInterface::Basic();
  auto rel_wind = info.wind;
  const auto &look = UIGlobals::GetLook().wind_arrow_info_box;
  rel_wind.bearing -= basic.attitude.heading;

  PaintWindArrow(canvas, rc, rel_wind,
                 info.wind_source == DerivedInfo::WindSource::EXTERNAL ?
                 look.arrow_brush_extern : look.arrow_brush);
  if (basic.external_instantaneous_wind_available) {
    rel_wind = basic.external_instantaneous_wind;
    rel_wind.bearing -= basic.attitude.heading;
    PaintWindArrow(canvas, rc, rel_wind,
                   look.arrow_brush_instantaneous);
  }
}
