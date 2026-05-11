// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapPreviewLayers.hpp"
#include "MapWindow/MapGroundLayers.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Interface.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "ui/canvas/Canvas.hpp"

MapPreviewLayers::MapPreviewLayers(const AirspaceLook &airspace_look,
                                   const TopographyLook &_topography_look) noexcept
  :airspace_renderer(airspace_look),
   topography_look(_topography_look)
{
}

MapPreviewLayers::~MapPreviewLayers() noexcept
{
  delete topography_renderer;
}

void
MapPreviewLayers::SetAirspaceWarnings(
  ProtectedAirspaceWarningManager *wm) noexcept
{
  airspace_warnings = wm;
  if (data_linked)
    airspace_renderer.SetAirspaceWarnings(airspace_warnings);
}

void
MapPreviewLayers::LinkFromDataSources() noexcept
{
  if (data_linked)
    return;

  if (data_components == nullptr)
    return;

  data_linked = true;

  airspace_renderer.SetAirspaceWarnings(airspace_warnings);

  background.SetTerrain(data_components->terrain.get());

  delete topography_renderer;
  topography_renderer =
    data_components->topography != nullptr
      ? new TopographyRenderer(*data_components->topography, topography_look)
      : nullptr;

  airspace_renderer.SetAirspaces(data_components->airspaces.get());
}

void
MapPreviewLayers::SetTopography(TopographyStore *topography) noexcept
{
  delete topography_renderer;
  topography_renderer =
    topography != nullptr
      ? new TopographyRenderer(*topography, topography_look)
      : nullptr;
}

void
MapPreviewLayers::RenderTerrain(Canvas &canvas,
                                const MapWindowProjection &projection) noexcept
{
  const auto &map_settings = CommonInterface::GetMapSettings();
  background.PaintTerrainLayer(canvas, projection, map_settings.terrain,
                               CommonInterface::Calculated(), true);
}

void
MapPreviewLayers::RenderTopography(Canvas &canvas,
                                   const MapWindowProjection &projection) noexcept
{
  MapGroundLayers::PaintTopographyLayer(
    topography_renderer,
    CommonInterface::GetMapSettings().topography_enabled,
    canvas, projection);
}

void
MapPreviewLayers::RenderTopographyLabels(Canvas &canvas,
                                         const MapWindowProjection &projection,
                                         LabelBlock &label_block) noexcept
{
  MapGroundLayers::PaintTopographyLabelsLayer(
    topography_renderer,
    CommonInterface::GetMapSettings().topography_enabled,
    canvas, projection, label_block);
}

#ifndef ENABLE_OPENGL
void
MapPreviewLayers::RenderAirspace(Canvas &canvas,
                                 BufferCanvas &stencil,
                                 const MapWindowProjection &projection) noexcept
{
  MapGroundLayers::PaintAirspaceMapLayers(
    airspace_renderer, nullptr, canvas, stencil, projection,
    CommonInterface::Basic(), CommonInterface::Calculated(),
    CommonInterface::GetComputerSettings().airspace,
    CommonInterface::GetMapSettings().airspace);
}
#else
void
MapPreviewLayers::RenderAirspace(Canvas &canvas,
                                 const MapWindowProjection &projection) noexcept
{
  MapGroundLayers::PaintAirspaceMapLayers(
    airspace_renderer, nullptr, canvas, projection,
    CommonInterface::Basic(), CommonInterface::Calculated(),
    CommonInterface::GetComputerSettings().airspace,
    CommonInterface::GetMapSettings().airspace);
}
#endif
