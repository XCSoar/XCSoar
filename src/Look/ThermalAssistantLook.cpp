/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "ThermalAssistantLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"

void
ThermalAssistantLook::Initialise(bool small)
{
  hcBackground = Color(0xFF, 0xFF, 0xFF);
  hcCircles = Color(0xB0, 0xB0, 0xB0);
  hcStandard = Color(0x00, 0x00, 0x00);
  hcPolygonBrush = Color(0xCC, 0xCC, 0xFF);
  hcPolygonPen = Color(0x00, 0x00, 0xFF);

  hbBackground.Set(hcBackground);

#ifdef ENABLE_OPENGL
  hbPolygon.Set(hcPolygonBrush.WithAlpha(128));
#else /* !OPENGL */
  hbPolygon.Set(hcPolygonBrush);
#endif /* !OPENGL */

  UPixelScalar width = Layout::FastScale(small ? 1 : 2);
#ifdef ENABLE_OPENGL
  hpPolygon.Set(width, hcPolygonPen.WithAlpha(128));
#else /* !OPENGL */
  hpPolygon.Set(width, hcPolygonPen);
#endif /* !OPENGL */
  hpInnerCircle.Set(1, hcCircles);
  hpOuterCircle.Set(Pen::DASH, 1, hcCircles);
  hpPlane.Set(width, COLOR_BLACK);

  hfNoTraffic.Set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  hfLabels.Set(Fonts::GetStandardFontFace(), Layout::FastScale(12));
}

void
ThermalAssistantLook::Deinitialise()
{
  hbBackground.Reset();
  hbPolygon.Reset();

  hpPlane.Reset();
  hpRadar.Reset();
  hpPolygon.Reset();
  hpInnerCircle.Reset();
  hpOuterCircle.Reset();

  hfLabels.Reset();
  hfNoTraffic.Reset();
}
