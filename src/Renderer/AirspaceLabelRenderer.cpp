// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceLabelRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Language/Language.hpp"
#include "Renderer/TextInBox.hpp"
#include "NMEA/Aircraft.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "LogFile.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"
#include "util/UTF8.hpp"
#include "Sizes.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

static constexpr double NOTAM_LABEL_MAX_MAP_SCALE = 4000;
static constexpr std::size_t NOTAM_LABEL_MAX_CHARS = 40;
static constexpr unsigned NOTAM_CLUSTER_VISIBLE_LINES = 3;
static constexpr unsigned NOTAM_CLUSTER_LABEL_LINES = 2;
static constexpr unsigned NOTAM_CLUSTER_ANCHOR_OFFSET = 18;
static constexpr unsigned NOTAM_CLUSTER_SCREEN_MARGIN = 12;
static constexpr std::size_t NOTAM_CLUSTER_MAX_COUNT = 64;

struct NotamLabelCluster {
  PixelPoint anchor;
  unsigned count = 0;
  StaticArray<StaticString<64>, NOTAM_CLUSTER_VISIBLE_LINES> labels;
};

class AirspaceMapVisible
{
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningCopy &warnings;

public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState &_state,
                     const AirspaceWarningCopy &_warnings) noexcept
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings) {}

  [[gnu::pure]]
  bool operator()(const AbstractAirspace& airspace) const noexcept {
    return visible_predicate(airspace) ||
      warnings.IsInside(airspace) ||
      warnings.HasWarning(airspace);
  }
};

static StaticString<64>
MakeNotamLabelText(const AbstractAirspace &airspace) noexcept
{
  const char *const name = airspace.GetName();
  if (name == nullptr || name[0] == '\0')
    return StaticString<64>{"NOTAM"};

  if (!ValidateUTF8(name))
    return StaticString<64>{"NOTAM"};

  StaticString<64> label;
  constexpr std::size_t max_bytes_without_ellipsis =
    64 - 1 - 3; // storage minus terminator and "..."
  const std::size_t length =
    TruncateStringUTF8(name, NOTAM_LABEL_MAX_CHARS,
                       max_bytes_without_ellipsis);
  std::copy_n(name, length, label.buffer());
  label.buffer()[length] = '\0';

  // NOTAM names may contain line breaks; replace them before rendering.
  std::replace(label.buffer(), label.buffer() + length, '\r', ' ');
  std::replace(label.buffer(), label.buffer() + length, '\n', ' ');

  if (name[length] != '\0')
    label += "...";

  return label;
}

[[gnu::pure]]
static unsigned
GetNotamClusterDistance() noexcept
{
  return Layout::Scale(20u);
}

[[gnu::pure]]
static unsigned
GetNotamLabelLineStep(const Canvas &canvas) noexcept
{
  return canvas.GetFontHeight() + Layout::GetTextPadding() + Layout::Scale(2u);
}

[[gnu::pure]]
static bool
MatchesNotamCluster(const NotamLabelCluster &cluster,
                    const PixelPoint pos,
                    const unsigned distance) noexcept
{
  return std::abs(pos.x - cluster.anchor.x) <= int(distance) &&
         std::abs(pos.y - cluster.anchor.y) <= int(distance);
}

static void
AddToNotamCluster(NotamLabelCluster &cluster,
                  const StaticString<64> &label) noexcept
{
  ++cluster.count;

  if (cluster.labels.size() < NOTAM_CLUSTER_VISIBLE_LINES)
    cluster.labels.append(label);
}

static StaticString<32>
MakeNotamOverflowLabel(const unsigned hidden_count) noexcept
{
  StaticString<32> summary;
  if (hidden_count == 1)
    summary = _("+ 1 NOTAM");
  else
    summary.Format(_("+ %u NOTAMs"), hidden_count);
  return summary;
}

[[gnu::pure]]
static PixelPoint
AdjustNotamClusterAnchor(const PixelPoint anchor,
                         const PixelRect &screen_rect) noexcept
{
  const int offset = Layout::Scale(NOTAM_CLUSTER_ANCHOR_OFFSET);
  const int margin = Layout::Scale(NOTAM_CLUSTER_SCREEN_MARGIN);

  if (anchor.y - offset > screen_rect.top + margin)
    return anchor.At(0, -offset);

  if (anchor.y + offset < screen_rect.bottom - margin)
    return anchor.At(0, offset);

  return anchor;
}

static void
DrawNotamCluster(Canvas &canvas,
                 const PixelPoint anchor,
                 const NotamLabelCluster &cluster,
                 const TextInBoxMode mode,
                 const PixelRect &screen_rect,
                 LabelBlock *label_block) noexcept
{
  const unsigned visible_lines = std::min(cluster.count,
                                          NOTAM_CLUSTER_VISIBLE_LINES);
  if (visible_lines == 0)
    return;

  const int line_step = GetNotamLabelLineStep(canvas);
  const int first_offset = -int((visible_lines - 1) * line_step) / 2;

  unsigned index = 0;
  const unsigned label_lines = cluster.count > NOTAM_CLUSTER_VISIBLE_LINES
    ? NOTAM_CLUSTER_LABEL_LINES
    : cluster.labels.size();

  for (; index < label_lines; ++index)
    TextInBox(canvas, cluster.labels[index].c_str(),
              anchor.At(0, first_offset + int(index * line_step)),
              mode, screen_rect, label_block);

  if (cluster.count > NOTAM_CLUSTER_VISIBLE_LINES) {
    const auto summary = MakeNotamOverflowLabel(cluster.count - label_lines);
    TextInBox(canvas, summary.c_str(),
              anchor.At(0, first_offset + int(index * line_step)),
              mode, screen_rect, label_block);
  }
}

void
AirspaceLabelRenderer::Draw(Canvas &canvas,
                            const WindowProjection &projection,
                            const MoreData &basic, const DerivedInfo &calculated,
                            const AirspaceComputerSettings &computer_settings,
                            const AirspaceRendererSettings &settings,
                            LabelBlock *label_block) noexcept
{
  const bool draw_altitude_labels =
    settings.label_selection == AirspaceRendererSettings::LabelSelection::ALL;
  const bool draw_notam_labels =
    settings.show_notam_labels &&
    projection.GetMapScale() <= NOTAM_LABEL_MAX_MAP_SCALE;

  if ((!draw_altitude_labels && !draw_notam_labels) ||
      airspaces == nullptr || airspaces->IsEmpty())
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != nullptr)
    awc.Visit(*warning_manager);

  const AircraftState aircraft = ToAircraftState(basic, calculated);
  const AirspaceMapVisible visible(computer_settings, settings,
                                   aircraft, awc);

  DrawInternal(canvas,
               projection, visible, computer_settings.warnings,
               draw_altitude_labels, draw_notam_labels, label_block);
}

inline void
AirspaceLabelRenderer::DrawInternal(Canvas &canvas,
                                    const WindowProjection &projection,
                                    AirspacePredicate visible,
                                    const AirspaceWarningConfig &config,
                                    const bool draw_altitude_labels,
                                    const bool draw_notam_labels,
                                    LabelBlock *label_block) noexcept
{
  AirspaceLabelList labels;

  if (draw_altitude_labels) {
    for (const auto &i : airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                                     projection.GetScreenDistanceMeters())) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        labels.Add(airspace.GetCenter(), airspace.GetClass(), airspace.GetBase(),
                   airspace.GetTop());
    }

    labels.Sort(config);
  }

  // default paint settings
  canvas.SetTextColor(look.label_text_color);
  canvas.Select(*look.name_font);
  canvas.Select(look.label_pen);
  canvas.Select(look.label_brush);
  canvas.SetBackgroundTransparent();

  if (draw_altitude_labels) {
    for (const auto &label : labels)
      DrawLabel(canvas, projection, label);
  }

  if (draw_notam_labels) {
    TextInBoxMode mode{};
    mode.shape = LabelShape::ROUNDED_WHITE;
    mode.align = TextInBoxMode::Alignment::CENTER;
    mode.vertical_position = TextInBoxMode::VerticalPosition::CENTERED;
    mode.move_in_view = true;

    StaticArray<NotamLabelCluster, NOTAM_CLUSTER_MAX_COUNT> clusters;
    const unsigned cluster_distance = GetNotamClusterDistance();

    for (const auto &i : airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                                     projection.GetScreenDistanceMeters())) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (!visible(airspace) ||
          airspace.GetType() != AirspaceClass::NOTAM)
        continue;

      const auto pos = projection.GeoToScreenIfVisible(airspace.GetCenter());
      if (!pos)
        continue;

      const auto label = MakeNotamLabelText(airspace);

      NotamLabelCluster *cluster = nullptr;
      for (auto &candidate : clusters)
        if (MatchesNotamCluster(candidate, *pos, cluster_distance)) {
          cluster = &candidate;
          break;
        }

      if (cluster == nullptr) {
        if (clusters.full()) {
#ifndef NDEBUG
          LogFmt("AirspaceLabelRenderer: skipped NOTAM label at {},{} "
                 "(cluster buffer full)",
                 pos->x, pos->y);
#endif
          continue;
        }

        cluster = &clusters.append();
        cluster->anchor = *pos;
        cluster->count = 0;
        cluster->labels.clear();
      }

      AddToNotamCluster(*cluster, label);
    }

    const PixelRect screen_rect = projection.GetScreenRect();
    for (const auto &cluster : clusters)
      DrawNotamCluster(canvas,
                       AdjustNotamClusterAnchor(cluster.anchor, screen_rect),
                       cluster, mode, screen_rect, label_block);
  }
}

inline void
AirspaceLabelRenderer::DrawLabel(Canvas &canvas,
                                 const WindowProjection &projection,
                                 const AirspaceLabelList::Label &label) noexcept
{
  char topText[NAME_SIZE + 1];
  AirspaceFormatter::FormatAltitudeShort(topText, label.top, false);
  const PixelSize topSize = canvas.CalcTextSize(topText);

  char baseText[NAME_SIZE + 1];
  AirspaceFormatter::FormatAltitudeShort(baseText, label.base, false);
  const PixelSize baseSize = canvas.CalcTextSize(baseText);

  const unsigned padding = Layout::GetTextPadding();
  const unsigned labelWidth =
    std::max(topSize.width, baseSize.width) + 2 * padding;
  const unsigned labelHeight = topSize.height + baseSize.height;

  // box
  const auto pos = projection.GeoToScreen(label.pos);
  PixelRect rect;
  rect.left = pos.x - labelWidth / 2;
  rect.top = pos.y;
  rect.right = rect.left + labelWidth;
  rect.bottom = rect.top + labelHeight;
  canvas.DrawRectangle(rect);

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

  // top text
  canvas.DrawText(rect.GetTopRight().At(-int(padding + topSize.width),
                                        0),
                  topText);

  // base text
  canvas.DrawText(rect.GetBottomRight().At(-int(padding + baseSize.width),
                                           -(int)baseSize.height),
                  baseText);
}
