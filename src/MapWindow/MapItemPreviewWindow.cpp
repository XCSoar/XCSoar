// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapItemPreviewWindow.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Renderer/TaskPointRenderer.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/MapScaleRenderer.hpp"
#include "Renderer/TrafficRenderer.hpp"
#include "Look/TrafficLook.hpp"
#include "Look/AirspaceLook.hpp"
#include "FLARM/Friends.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Interface.hpp"
#include "Computer/GlideComputer.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/ObservationZones/ObservationZonePoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Screen/Layout.hpp"
#include "FLARM/Traffic.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoVector.hpp"

#include <vector>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#else
#include "ui/canvas/WindowCanvas.hpp"
#endif

static const ComputerSettings &
GetComputerSettings() noexcept
{
  return CommonInterface::GetComputerSettings();
}

static const MapSettings &
GetMapSettings() noexcept
{
  return CommonInterface::GetMapSettings();
}

static const MoreData &
Basic() noexcept
{
  return CommonInterface::Basic();
}

static const DerivedInfo &
Calculated() noexcept
{
  return CommonInterface::Calculated();
}

MapItemPreviewWindow::MapItemPreviewWindow(const WaypointLook &waypoint_look,
                                           const AirspaceLook &_airspace_look,
                                           const TrailLook &_trail_look,
                                           const TaskLook &_task_look,
                                           const AircraftLook &_aircraft_look,
                                           const TopographyLook &_topography_look,
                                           const OverlayLook &_overlay_look,
                                           const TrafficLook &_traffic_look) noexcept
  :task_look(_task_look),
   aircraft_look(_aircraft_look),
   topography_look(_topography_look),
   overlay_look(_overlay_look),
   traffic_look(_traffic_look),
   airspace_renderer(_airspace_look),
   way_point_renderer(nullptr, waypoint_look),
   trail_renderer(_trail_look)
{
}

MapItemPreviewWindow::~MapItemPreviewWindow() noexcept
{
  Destroy();

  delete topography_renderer;
}

void
MapItemPreviewWindow::Create(ContainerWindow &parent, PixelRect rc,
                             WindowStyle style) noexcept
{
  projection.SetScale(0.01);

  BufferWindow::Create(parent, rc, style);
}

inline void
MapItemPreviewWindow::RenderTerrain(Canvas &canvas) noexcept
{
  background.SetShadingAngle(projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, projection, GetMapSettings().terrain);
}

inline void
MapItemPreviewWindow::RenderTopography(Canvas &canvas) noexcept
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, projection);
}

inline void
MapItemPreviewWindow::RenderTopographyLabels(Canvas &canvas) noexcept
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, projection, label_block);
}

inline void
MapItemPreviewWindow::RenderAirspace(Canvas &canvas) noexcept
{
  if (GetMapSettings().airspace.enable)
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas,
#endif
                           projection,
                           Basic(), Calculated(),
                           GetComputerSettings().airspace,
                           GetMapSettings().airspace);
}

inline void
MapItemPreviewWindow::DrawHighlightedAirspace(Canvas &canvas) noexcept
{
  if (highlight_airspace == nullptr)
    return;

  const AirspaceLook &look = airspace_renderer.GetLook();
  const MapSettings &settings_map = GetMapSettings();
  const AirspaceRendererSettings &settings = settings_map.airspace;

  AirspaceClass as_type_or_class = settings.classes[highlight_airspace->GetTypeOrClass()].display
    ? highlight_airspace->GetTypeOrClass()
    : highlight_airspace->GetClass();

  const AirspaceClassLook &class_look = look.classes[as_type_or_class];
  const AirspaceClassRendererSettings &class_settings = settings.classes[as_type_or_class];

  // Fill the airspace with color (like when it has a warning)
#ifdef ENABLE_OPENGL
  const GLEnable<GL_BLEND> blend;
  canvas.Select(Brush(class_look.fill_color.WithAlpha(90)));
#else
#ifdef HAVE_ALPHA_BLEND
  if (settings.transparency) {
    canvas.Select(class_look.solid_brush);
  } else {
#endif
#ifdef HAVE_HATCHED_BRUSH
    canvas.SetTextColor(LightColor(class_look.fill_color));
    canvas.Select(look.brushes[class_settings.brush]);
    canvas.SetBackgroundOpaque();
    canvas.SetBackgroundColor(COLOR_WHITE);
#else
    canvas.Select(class_look.solid_brush);
#endif
#ifdef HAVE_ALPHA_BLEND
  }
#endif
#endif
  canvas.SelectNullPen();

  switch (highlight_airspace->GetShape()) {
  case AbstractAirspace::Shape::CIRCLE: {
    const AirspaceCircle &circle = (const AirspaceCircle &)*highlight_airspace;
    const auto center = projection.GeoToScreen(circle.GetReferenceLocation());
    const unsigned radius = projection.GeoToScreenDistance(circle.GetRadius());
    canvas.DrawCircle(center, radius);
    break;
  }

  case AbstractAirspace::Shape::POLYGON: {
    const AirspacePolygon &polygon = (const AirspacePolygon &)*highlight_airspace;
    const SearchPointVector &points = polygon.GetPoints();
    if (points.empty())
      break;

    // Convert to screen coordinates
    std::vector<BulkPixelPoint> screen_points;
    screen_points.reserve(points.size());
    for (const auto &point : points)
      screen_points.push_back(projection.GeoToScreen(point.GetLocation()));

    canvas.DrawPolygon(screen_points.data(), screen_points.size());
    break;
  }
  }

  // Draw outline with border pen in airspace color (respecting black_outline setting)
  if (settings.black_outline) {
    canvas.SelectBlackPen();
  } else if (class_settings.border_width > 0) {
    canvas.Select(class_look.border_pen);
  } else {
    // No border if border_width is 0
    return;
  }
  canvas.SelectHollowBrush();

  switch (highlight_airspace->GetShape()) {
  case AbstractAirspace::Shape::CIRCLE: {
    const AirspaceCircle &circle = (const AirspaceCircle &)*highlight_airspace;
    const auto center = projection.GeoToScreen(circle.GetReferenceLocation());
    const unsigned radius = projection.GeoToScreenDistance(circle.GetRadius());
    canvas.DrawCircle(center, radius);
    break;
  }

  case AbstractAirspace::Shape::POLYGON: {
    const AirspacePolygon &polygon = (const AirspacePolygon &)*highlight_airspace;
    const SearchPointVector &points = polygon.GetPoints();
    if (points.empty())
      break;

    // Convert to screen coordinates
    std::vector<BulkPixelPoint> screen_points;
    screen_points.reserve(points.size());
    for (const auto &point : points)
      screen_points.push_back(projection.GeoToScreen(point.GetLocation()));

    canvas.DrawPolygon(screen_points.data(), screen_points.size());
    break;
  }
  }

  // Draw airspace label at center
  DrawAirspaceLabel(canvas);
}

inline void
MapItemPreviewWindow::DrawAirspaceLabel(Canvas &canvas) noexcept
{
  if (highlight_airspace == nullptr)
    return;

  const AirspaceLook &look = airspace_renderer.GetLook();

  // Format altitude text
  TCHAR topText[40];
  AirspaceFormatter::FormatAltitudeShort(topText, highlight_airspace->GetTop(), false);
  const PixelSize topSize = canvas.CalcTextSize(topText);

  TCHAR baseText[40];
  AirspaceFormatter::FormatAltitudeShort(baseText, highlight_airspace->GetBase(), false);
  const PixelSize baseSize = canvas.CalcTextSize(baseText);

  const unsigned padding = Layout::GetTextPadding();
  const unsigned labelWidth =
    std::max(topSize.width, baseSize.width) + 2 * padding;
  const unsigned labelHeight = topSize.height + baseSize.height;

  // Get airspace center position
  const auto center_geo = highlight_airspace->GetCenter();
  const auto pos = projection.GeoToScreen(center_geo);

  // Calculate label rectangle
  PixelRect rect;
  rect.left = pos.x - labelWidth / 2;
  rect.top = pos.y;
  rect.right = rect.left + labelWidth;
  rect.bottom = rect.top + labelHeight;

  // Draw label background
  canvas.SetTextColor(look.label_text_color);
  canvas.Select(*look.name_font);
  canvas.Select(look.label_pen);
  canvas.Select(look.label_brush);
  canvas.SetBackgroundTransparent();
  canvas.DrawRectangle(rect);

  // Draw separator line
#ifdef USE_GDI
  canvas.DrawLine(rect.left + padding,
                  rect.top + labelHeight / 2,
                  rect.right - padding,
                  rect.top + labelHeight / 2);
#else
  canvas.DrawHLine(rect.left + padding,
                   rect.right - padding,
                   rect.top + labelHeight / 2, look.label_pen.GetColor());
#endif

  // Draw top altitude text
  canvas.DrawText(rect.GetTopRight().At(-int(padding + topSize.width),
                                        0),
                  topText);

  // Draw base altitude text
  canvas.DrawText(rect.GetBottomRight().At(-int(padding + baseSize.width),
                                           -(int)baseSize.height),
                  baseText);
}

inline void
MapItemPreviewWindow::DrawTask(Canvas &canvas) noexcept
{
  if (task == nullptr)
    return;

  ProtectedTaskManager::Lease task_manager(*task);
  const AbstractTask *task = task_manager->GetActiveTask();
  if (task && !IsError(task->CheckTask())) {
    const OrderedTask &o_task = task_manager->GetOrderedTask();
    if (o_task.IsEmpty())
      return;

    OZRenderer ozv(task_look, airspace_renderer.GetLook(),
                   GetMapSettings().airspace);
    TaskPointRenderer tpv(canvas, projection, task_look,
                          o_task.GetTaskProjection(),
                          ozv, false,
                          TaskPointRenderer::TargetVisibility::ALL,
                          Basic().GetLocationOrInvalid());
    tpv.SetTaskFinished(Calculated().task_stats.task_finished);
    TaskRenderer dv(tpv, projection.GetScreenBounds());
    dv.Draw(*task);
  }
}

inline void
MapItemPreviewWindow::DrawWaypoints(Canvas &canvas) noexcept
{
  const MapSettings &settings_map = GetMapSettings();
  WaypointRendererSettings settings = settings_map.waypoint;
  settings.display_text_type = WaypointRendererSettings::DisplayTextType::NAME;

  way_point_renderer.Render(canvas, label_block,
                            projection, settings,
                            GetComputerSettings().polar,
                            GetComputerSettings().task,
                            Basic(), Calculated(),
                            task, nullptr);
}

inline void
MapItemPreviewWindow::RenderTrail(Canvas &canvas) noexcept
{
  if (glide_computer == nullptr)
    return;

  const auto min_time = std::max(Basic().time - std::chrono::minutes{10},
                                 TimeStamp{});
  trail_renderer.Draw(canvas, glide_computer->GetTraceComputer(),
                      projection, min_time);
}

inline void
MapItemPreviewWindow::DrawTraffic(Canvas &canvas) noexcept
{
  if (current_item == nullptr ||
      current_item->type != MapItem::Type::TRAFFIC)
    return;

  const TrafficMapItem &traffic_item = (const TrafficMapItem &)*current_item;
  const FlarmTraffic *traffic =
    Basic().flarm.traffic.FindTraffic(traffic_item.id);
  if (traffic == nullptr || !traffic->location_available)
    return;

  const auto sc = projection.GeoToScreen(traffic->location);
  auto color = FlarmFriends::GetFriendColor(traffic->id);
  if (traffic_item.color != FlarmColor::NONE)
    color = traffic_item.color;

  TrafficRenderer::Draw(canvas, traffic_look, false, *traffic,
                        traffic->track - projection.GetScreenAngle(),
                        color, sc);
}

void
MapItemPreviewWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
  GLCanvasScissor scissor(canvas);
#endif

  const auto aircraft_pos = projection.GeoToScreen(Basic().location);

  label_block.reset();

  RenderTerrain(canvas);
  RenderTopography(canvas);
  RenderAirspace(canvas);

#ifdef ENABLE_OPENGL
  canvas.FadeToWhite(0x80);
#endif

  DrawTask(canvas);
  DrawWaypoints(canvas);
  DrawTraffic(canvas);
  RenderTrail(canvas);

  RenderTopographyLabels(canvas);

  // Draw highlighted airspace on top
  DrawHighlightedAirspace(canvas);

  // Draw line and target for LOCATION items
  if (current_item != nullptr && current_item->type == MapItem::Type::LOCATION &&
      target_location.IsValid() && Basic().location_available) {
    const auto target_pos = projection.GeoToScreen(target_location);
    
    // Draw line from current position to target
    canvas.SelectBlackPen();
    canvas.DrawLine(aircraft_pos, target_pos);
    
    // Draw target marker (circle)
    canvas.SelectHollowBrush();
    canvas.DrawCircle(target_pos, Layout::Scale(8));
  }

  if (Basic().alive)
    AircraftRenderer::Draw(canvas, GetMapSettings(), aircraft_look,
                           Basic().attitude.heading - projection.GetScreenAngle(),
                           aircraft_pos);

  RenderMapScale(canvas, projection, GetClientRect(), overlay_look);
}

void
MapItemPreviewWindow::SetTerrain(RasterTerrain *terrain) noexcept
{
  background.SetTerrain(terrain);
}

void
MapItemPreviewWindow::SetTopograpgy(TopographyStore *topography) noexcept
{
  delete topography_renderer;
  topography_renderer = topography != nullptr
    ? new TopographyRenderer(*topography, topography_look)
    : nullptr;
}

[[gnu::pure]]
static double
GetRadius(const ObservationZonePoint &oz) noexcept
{
  switch (oz.GetShape()) {
  case ObservationZone::Shape::LINE:
  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::CYLINDER:
  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::FAI_SECTOR:
  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
  case ObservationZone::Shape::BGA_START:
  case ObservationZone::Shape::ANNULAR_SECTOR:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    const CylinderZone &cz = (const CylinderZone &)oz;
    return cz.GetRadius();
  }

  return 1;
}

void
MapItemPreviewWindow::SetMapItem(const MapItem &item) noexcept
{
  current_item = &item;
  highlight_airspace = nullptr;
  target_location.SetInvalid();

  GeoPoint location;
  double radius = 5000.; // Default 5km radius

  switch (item.type) {
  case MapItem::Type::AIRSPACE: {
    const AirspaceMapItem &as_item = (const AirspaceMapItem &)item;
    const AbstractAirspace &airspace = *as_item.airspace;
    highlight_airspace = &airspace;

    GeoBounds bounds = airspace.GetGeoBounds();
    location = bounds.GetCenter();

    // Calculate radius to fit the airspace
    const auto geo_height = bounds.GetGeoHeight();
    const auto geo_width = bounds.GetGeoWidth();
    const auto geo_size = std::max(geo_height, geo_width);
    radius = std::max(geo_size / 2. * 1.3, 2000.);

    // For circular airspaces, use the actual radius
    if (airspace.GetShape() == AbstractAirspace::Shape::CIRCLE) {
      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      radius = std::max(circle.GetRadius() * 1.3, 2000.);
    }
    break;
  }

  case MapItem::Type::WAYPOINT: {
    const WaypointMapItem &wp_item = (const WaypointMapItem &)item;
    location = wp_item.waypoint->location;
    radius = 5000.; // 5km zoom for waypoints
    break;
  }

  case MapItem::Type::TRAFFIC: {
    const TrafficMapItem &traffic_item = (const TrafficMapItem &)item;
    const FlarmTraffic *traffic =
      Basic().flarm.traffic.FindTraffic(traffic_item.id);
    if (traffic != nullptr && traffic->location_available) {
      location = traffic->location;
      radius = 5000.; // 5km zoom for traffic
    } else {
      return; // Can't show traffic without location
    }
    break;
  }

  case MapItem::Type::TASK_OZ: {
    const TaskOZMapItem &oz_item = (const TaskOZMapItem &)item;
    if (oz_item.oz != nullptr) {
      location = oz_item.oz->GetReference();
      radius = std::max(GetRadius(*oz_item.oz) * 1.3, 2000.);
    } else {
      return;
    }
    break;
  }

#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER: {
    const WeatherStationMapItem &weather_item = (const WeatherStationMapItem &)item;
    if (weather_item.station->parsed_metar_available &&
        weather_item.station->parsed_metar.location_available) {
      location = weather_item.station->parsed_metar.location;
      radius = 10000.; // 10km zoom for weather stations
    } else {
      return; // Can't show weather station without location
    }
    break;
  }
#endif

  case MapItem::Type::LOCATION: {
    const LocationMapItem &loc_item = (const LocationMapItem &)item;
    if (Basic().location_available && loc_item.vector.IsValid()) {
      // Calculate target location from vector
      target_location = loc_item.vector.EndPoint(Basic().location);
      
      // Center map between current position and target
      const GeoPoint center = Basic().location.Interpolate(target_location, 0.5);
      location = center;
      
      // Set radius to show both points with some margin
      radius = std::max(loc_item.vector.distance * 0.6, 2000.);
    } else {
      target_location.SetInvalid();
      return;
    }
    break;
  }

  case MapItem::Type::ARRIVAL_ALTITUDE: {
    // Use the clicked location from Basic
    if (Basic().location_available) {
      location = Basic().location;
      radius = 5000.;
    } else {
      return;
    }
    break;
  }

  case MapItem::Type::SELF:
  case MapItem::Type::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
  case MapItem::Type::OVERLAY:
  case MapItem::Type::RASP:
    // These don't have a specific location to zoom to
    if (Basic().location_available) {
      location = Basic().location;
      radius = 10000.;
    } else {
      return;
    }
    break;
  }

  projection.SetGeoLocation(location);
  projection.SetScaleFromRadius(radius);
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  Invalidate();
}

void
MapItemPreviewWindow::OnResize(PixelSize new_size) noexcept
{
  BufferWindow::OnResize(new_size);

#ifndef ENABLE_OPENGL
  buffer_canvas.Grow(new_size);
#endif

  projection.SetScreenSize(new_size);
  projection.SetScreenOrigin(PixelRect{new_size}.GetCenter());
  projection.UpdateScreenBounds();
}

void
MapItemPreviewWindow::OnCreate()
{
  BufferWindow::OnCreate();

#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  buffer_canvas.Create(canvas);
#endif
}

void
MapItemPreviewWindow::OnDestroy() noexcept
{
  SetTerrain(nullptr);
  SetTopograpgy(nullptr);
  SetAirspaces(nullptr);
  SetWaypoints(nullptr);

#ifndef ENABLE_OPENGL
  buffer_canvas.Destroy();
#endif

  BufferWindow::OnDestroy();
}

