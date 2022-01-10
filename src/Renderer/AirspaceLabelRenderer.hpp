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

#ifndef XCSOAR_AIRSPACE_LABEL_RENDERER_HPP
#define XCSOAR_AIRSPACE_LABEL_RENDERER_HPP

#include "AirspaceLabelList.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"

struct AirspaceLook;
struct MoreData;
struct DerivedInfo;
struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
struct AirspaceWarningConfig;
class Airspaces;
class ProtectedAirspaceWarningManager;
class Canvas;
class WindowProjection;

class AirspaceLabelRenderer
{
  const AirspaceLook &look;
  const Airspaces *airspaces = nullptr;
  const ProtectedAirspaceWarningManager *warning_manager = nullptr;

public:
  explicit AirspaceLabelRenderer(const AirspaceLook &_look) noexcept
    :look(_look) {}

  const AirspaceLook &GetLook() const noexcept {
    return look;
  }

  const Airspaces *GetAirspaces() const noexcept {
    return airspaces;
  }

  const ProtectedAirspaceWarningManager *GetWarningManager() const noexcept {
    return warning_manager;
  }

  void SetAirspaces(const Airspaces *_airspaces) noexcept {
    airspaces = _airspaces;
  }

  void SetAirspaceWarnings(const ProtectedAirspaceWarningManager *_warning_manager) noexcept {
    warning_manager = _warning_manager;
  }

  void Clear() noexcept {
    airspaces = nullptr;
    warning_manager = nullptr;
  }

private:
  void DrawInternal(Canvas &canvas,
                    const WindowProjection &projection,
                    AirspacePredicate visible,
                    const AirspaceWarningConfig &config) noexcept;

  void DrawLabel(Canvas &canvas, const WindowProjection &projection,
                 const AirspaceLabelList::Label &label) noexcept;

public:
   /**
   * Draw labels that are visible according to standard rules.
   */
  void Draw(Canvas &canvas,
            const WindowProjection &projection,
            const MoreData &basic, const DerivedInfo &calculated,
            const AirspaceComputerSettings &computer_settings,
            const AirspaceRendererSettings &settings) noexcept;
};

#endif
