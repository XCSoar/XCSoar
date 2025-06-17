// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Color.hpp"
#include "FLARM/TrafficClimbAltIndicators.hpp"

struct PixelPoint;
class Canvas;
struct TrafficLook;
struct FlarmTraffic;
struct GliderLinkTraffic;
class Angle;

namespace TrafficRenderer
{
void
Draw(Canvas &canvas, const TrafficLook &traffic_look,
     bool fading, bool colorful_traffic,
     const FlarmTraffic &traffic, Angle angle,
     FlarmColor color, PixelPoint pt,
     TrafficClimbAltIndicators indicators) noexcept;   
     
void 
Draw(Canvas &canvas, const TrafficLook &traffic_look,
     const GliderLinkTraffic &traffic, Angle angle, PixelPoint pt) noexcept;
}
