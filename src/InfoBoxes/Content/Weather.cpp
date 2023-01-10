/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
  data.UnsafeFormatValue( _T("%d"), (int)basic.humidity);
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
  data.SetValue(_T("%2.1f"), basic.temperature.ToUser());

  data.SetValueUnit(Units::current.temperature_unit);
}

void
InfoBoxContentTemperatureForecast::Update(InfoBoxData &data) noexcept
{
  auto temperature = CommonInterface::GetComputerSettings().forecast_temperature;
  data.SetValue(_T("%2.1f"), temperature.ToUser());

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
  data.SetValue(_T("%2.0f"),
                    Units::ToUserWindSpeed(info.wind.norm));

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
  data.SetValue(_T("%2.0f"),
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
  data.SetValue(_T("%2.0f"),
                    Units::ToUserWindSpeed(info.head_wind));

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
  data.SetValue(_T("%2.0f"), Units::ToUserWindSpeed(value));

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
PaintWindArrow(Canvas &canvas, const PixelRect &rc, const SpeedVector &wind)
{
  constexpr unsigned arrow_width = 6;
  constexpr unsigned arrow_tail_length = 3;

  const auto pt = rc.GetCenter();

  const unsigned scale = Layout::Scale(100U);

  const unsigned padding = Layout::FastScale(10u);
  unsigned size = std::min(rc.GetWidth(), rc.GetHeight());

  if (size > padding)
    size -= padding;

  // Normalize the size because the Layout::Scale is applied
  // by the DrawArrow() function again
  const int length = std::min(size, std::max(10u, uround(4 * wind.norm)));

  const int offset = -length / 2;

  const auto style = CommonInterface::GetMapSettings().wind_arrow_style;

  WindArrowRenderer renderer(UIGlobals::GetLook().wind_arrow_info_box);
  renderer.DrawArrow(canvas, pt, wind.bearing, arrow_width, length,
                     arrow_tail_length, style, offset, scale);
}

void
InfoBoxContentWindArrow::OnCustomPaint(Canvas &canvas,
                                       const PixelRect &rc) noexcept
{
 
  const auto &info = CommonInterface::Calculated();
  const auto &basic = CommonInterface::Basic();
  auto rel_wind = info.wind;
  rel_wind.bearing -= basic.attitude.heading;
  PaintWindArrow(canvas, rc, rel_wind);
  if (basic.external_instantaneous_wind_available) {
    rel_wind = basic.external_instantaneous_wind;
    rel_wind.bearing -= basic.attitude.heading;
    PaintWindArrow(canvas, rc, rel_wind);
  }
}
