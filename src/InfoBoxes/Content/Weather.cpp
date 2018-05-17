/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Renderer/WindArrowRenderer.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#include <tchar.h>

void
UpdateInfoBoxHumidity(InfoBoxData &data)
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
UpdateInfoBoxTemperature(InfoBoxData &data)
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
InfoBoxContentTemperatureForecast::Update(InfoBoxData &data)
{
  auto temperature = CommonInterface::GetComputerSettings().forecast_temperature;
  data.SetValue(_T("%2.1f"), temperature.ToUser());

  data.SetValueUnit(Units::current.temperature_unit);
}

bool
InfoBoxContentTemperatureForecast::HandleKey(const InfoBoxKeyCodes keycode)
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
InfoBoxContentWindArrow::GetDialogContent()
{
  return wind_infobox_panels;
}

void
UpdateInfoBoxWindSpeed(InfoBoxData &data)
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
UpdateInfoBoxWindBearing(InfoBoxData &data)
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

void
UpdateInfoBoxHeadWind(InfoBoxData &data)
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
UpdateInfoBoxHeadWindSimplified(InfoBoxData &data)
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
InfoBoxContentWindArrow::Update(InfoBoxData &data)
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available || info.wind.IsZero()) {
    data.SetInvalid();
    return;
  }

  data.SetCustom();

  TCHAR speed_buffer[16];
  FormatUserWindSpeed(info.wind.norm, speed_buffer, true, false);

  StaticString<32> buffer;
  buffer.Format(_T("%s / %s"),
                FormatBearing(info.wind.bearing).c_str(),
                speed_buffer);
  data.SetComment(buffer);
}

void
InfoBoxContentWindArrow::OnCustomPaint(Canvas &canvas, const PixelRect &rc)
{
  const auto &info = CommonInterface::Calculated();

  const auto pt = rc.GetCenter();

  const unsigned padding = Layout::FastScale(10u);
  unsigned size = std::min(rc.GetWidth(), rc.GetHeight());

  if (size > padding)
    size -= padding;

  // Normalize the size because the Layout::Scale is applied
  // by the DrawArrow() function again
  size = size * 100 / Layout::Scale(100);

  auto angle = info.wind.bearing - CommonInterface::Basic().attitude.heading;

  const int length =
    std::min(size, std::max(10u, uround(4 * info.wind.norm)));

  const int offset = -length / 2;

  auto style = CommonInterface::GetMapSettings().wind_arrow_style;

  WindArrowRenderer renderer(UIGlobals::GetLook().wind_arrow_info_box);
  renderer.DrawArrow(canvas, pt, angle, length, style, offset);
}
