// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct Look;
namespace UI { class SingleWindow; }
class FullBlackboard;
class GlideComputer;
class Airspaces;
class TerrainBase;

enum class AnalysisPage {
  BAROGRAPH,
  CLIMB,
  THERMAL_BAND,
  VARIO_HISTOGRAM,
  TASK_SPEED,
  WIND,
  POLAR,
  MACCREADY,
  TEMPTRACE,
  TASK,
  CONTEST,
  AIRSPACE,
  COUNT
};

void
dlgAnalysisShowModal(UI::SingleWindow &parent, const Look &look,
                     const FullBlackboard &blackboard,
                     GlideComputer &glide_computer,
                     const Airspaces *airspaces,
                     const TerrainBase *terrain,
                     AnalysisPage page=AnalysisPage::COUNT);
