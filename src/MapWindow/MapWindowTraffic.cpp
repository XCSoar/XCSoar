// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "ui/canvas/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Formatter/UserUnits.hpp"
#include "Look/TrafficLook.hpp"
#include "Renderer/TextInBox.hpp"
#include "Renderer/TrafficRenderer.hpp"
#include "FLARM/Friends.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

static void
DrawFlarmTraffic(Canvas &canvas, const WindowProjection &projection,
                 const TrafficLook &look, bool fading,
                 const PixelPoint aircraft_pos,
                 const FlarmTraffic &traffic) noexcept
{
  assert(traffic.location_available);

  // Points for the screen coordinates for the icon, name and average climb
  PixelPoint sc;

  // If FLARM target not on the screen, move to the next one
  if (auto p = projection.GeoToScreenIfVisible(traffic.location))
    sc = *p;
  else
    return;

  TextInBoxMode mode;
  if (!fading)
    mode.shape = LabelShape::OUTLINED;

  // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

  // only draw labels if not close to aircraft
  if ((sc - aircraft_pos).MagnitudeSquared() > Layout::Scale(30 * 30)) {
    // If FLARM callsign/name available draw it to the canvas
    if (traffic.HasName() && !StringIsEmpty(traffic.name)) {
      // Draw the name 16 points below the icon
      auto sc_name = sc;
      sc_name.y -= Layout::Scale(20);

      TextInBox(canvas, traffic.name, sc_name,
                mode, projection.GetScreenRect());
    }

    if (!fading && traffic.climb_rate_avg30s >= 0.1) {
      // If average climb data available draw it to the canvas

      // Draw the average climb value above the icon
      auto sc_av = sc;
      sc_av.y += Layout::Scale(5);

      TextInBox(canvas,
                FormatUserVerticalSpeed(traffic.climb_rate_avg30s, false),
                sc_av, mode,
                projection.GetScreenRect());
    }
  }

  auto color = FlarmFriends::GetFriendColor(traffic.id);

  TrafficRenderer::Draw(canvas, look, fading, traffic,
                        traffic.track - projection.GetScreenAngle(),
                        color, sc);
}

/**
 * Draws the FLARM traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawFLARMTraffic(Canvas &canvas,
                            const PixelPoint aircraft_pos) const noexcept
{
  // Return if FLARM icons on moving map are disabled
  if (!GetMapSettings().show_flarm_on_map)
    return;

  // Return if FLARM data is not available
  const TrafficList &flarm = Basic().flarm.traffic;

  const WindowProjection &projection = render_projection;

  // if zoomed in too far out, dont draw traffic since it will be too close to
  // the glider and so will be meaningless (serves only to clutter, cant help
  // the pilot)
  if (projection.GetMapScale() > 7300)
    return;

  canvas.Select(*traffic_look.font);

  // Circle through the FLARM targets
  for (const auto &traffic : flarm.list) {
    if (!traffic.location_available)
      continue;

  // No position traffic (relative_east=0) does not make sense in map display
    if (traffic.relative_east)
      DrawFlarmTraffic(canvas, projection, traffic_look, false,
                       aircraft_pos, traffic);
  }

  if (const auto &fading = GetFadingFlarmTraffic(); !fading.empty()) {
    for (const auto &[id, traffic] : fading) {
      assert(traffic.location_available);

  // No position traffic (relative_east=0) does not make sense in map display
      if (traffic.relative_east)
        DrawFlarmTraffic(canvas, projection, traffic_look, true,
                         aircraft_pos, traffic);
    }
  }
}


/**
 * Draws the GliderLink traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawGLinkTraffic([[maybe_unused]] Canvas &canvas) const noexcept
{
#ifdef ANDROID

  // Return if FLARM icons on moving map are disabled
  if (!GetMapSettings().show_flarm_on_map)
    return;

  const GliderLinkTrafficList &traffic = Basic().glink_data.traffic;
  if (traffic.IsEmpty())
    return;

  const MoreData &basic = Basic();

  const WindowProjection &projection = render_projection;

  canvas.Select(*traffic_look.font);

  // Circle through the GliderLink targets
  for (const auto &traf : traffic.list) {

    // Points for the screen coordinates for the icon, name and average climb
    PixelPoint sc;

    // If FLARM target not on the screen, move to the next one
    if (auto p = projection.GeoToScreenIfVisible(traf.location))
      sc = *p;
    else
      continue;

    TextInBoxMode mode;
    mode.shape = LabelShape::OUTLINED;
    mode.align = TextInBoxMode::Alignment::RIGHT;

    // If callsign/name available draw it to the canvas
    if (traf.HasName() && !StringIsEmpty(traf.name)) {
      // Draw the callsign above the icon
      auto sc_name = sc;
      sc_name.x -= Layout::Scale(10);
      sc_name.y -= Layout::Scale(15);

      TextInBox(canvas, traf.name, sc_name,
                mode, GetClientRect());
    }

    if (traf.climb_rate_received) {

      // If average climb data available draw it to the canvas
      mode.align = TextInBoxMode::Alignment::LEFT;

      // Draw the average climb to the right of the icon
      auto sc_av = sc;
      sc_av.x += Layout::Scale(10);
      sc_av.y -= Layout::Scale(8);

      TextInBox(canvas,
                FormatUserVerticalSpeed(traf.climb_rate, false),
                sc_av, mode, GetClientRect());
    }

    // use GPS altitude to be consistent with GliderLink
    if(basic.gps_altitude_available && traf.altitude_received
        && fabs(double(traf.altitude) - basic.gps_altitude) >= 100.0) {
      // If average climb data available draw it to the canvas
      char label_alt[100];
      double alt = (double(traf.altitude) - basic.gps_altitude) / 100.0;
      FormatRelativeUserAltitude(alt, label_alt, false);

      // Location of altitude label
      auto sc_alt = sc;
      sc_alt.x -= Layout::Scale(10);
      sc_alt.y -= Layout::Scale(0);

      mode.align = TextInBoxMode::Alignment::RIGHT;
      TextInBox(canvas, label_alt, sc_alt, mode, GetClientRect());
    }

    TrafficRenderer::Draw(canvas, traffic_look, traf,
                          traf.track - projection.GetScreenAngle(), sc);
  }
#endif
}

/**
 * Draws the teammate icon to the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawTeammate(Canvas &canvas) const noexcept
{
  const TeamInfo &teamcode_info = Calculated();

  if (teamcode_info.teammate_available) {
    if (auto p = render_projection.GeoToScreenIfVisible(teamcode_info.teammate_location))
      traffic_look.teammate_icon.Draw(canvas, *p);
  }
}

#ifdef HAVE_SKYLINES_TRACKING

void
MapWindow::DrawSkyLinesTraffic(Canvas &canvas) const noexcept
{
  if (DisplaySkyLinesTrafficMapMode::OFF == GetMapSettings().skylines_traffic_map_mode ||
      skylines_data == nullptr)
    return;

  canvas.Select(*traffic_look.font);

  const std::lock_guard lock{skylines_data->mutex};
  for (auto &i : skylines_data->traffic) {
    if (auto p = render_projection.GeoToScreenIfVisible(i.second.location)) {
      traffic_look.teammate_icon.Draw(canvas, *p);
      if (DisplaySkyLinesTrafficMapMode::SYMBOL_NAME == GetMapSettings().skylines_traffic_map_mode) {
        const auto name_i = skylines_data->user_names.find(i.first);
        const char *name = name_i != skylines_data->user_names.end()
          ? name_i->second.c_str()
          : "";

        StaticString<128> buffer;
        buffer.Format("%s [%um]", name, i.second.altitude);

        TextInBoxMode mode;
        mode.shape = LabelShape::OUTLINED;

        // Draw the name 16 points below the icon
        p->y -= Layout::Scale(10);
        TextInBox(canvas, buffer, *p, mode, GetClientRect());
      }
    }
  }
}

#endif
