// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DialogLook.hpp"
#include "GestureLook.hpp"
#include "TerminalLook.hpp"
#include "VarioLook.hpp"
#include "ChartLook.hpp"
#include "ThermalBandLook.hpp"
#include "TraceHistoryLook.hpp"
#include "MapLook.hpp"
#include "CrossSectionLook.hpp"
#include "HorizonLook.hpp"
#include "TrafficLook.hpp"
#include "FlarmTrafficLook.hpp"
#include "InfoBoxLook.hpp"
#include "FinalGlideBarLook.hpp"
#include "VarioBarLook.hpp"
#include "IconLook.hpp"
#include "ThermalAssistantLook.hpp"
#include "ClimbPercentLook.hpp"

struct UISettings;
class Font;

struct Look {
  DialogLook dialog;
  GestureLook gesture;
  TerminalLook terminal;
  VarioLook vario;
  ChartLook chart;
  ThermalBandLook thermal_band;
  TraceHistoryLook trace_history;
  MapLook map;
  CrossSectionLook cross_section;
  HorizonLook horizon;
  TrafficLook traffic;
  FlarmTrafficLook flarm_gauge;
  FlarmTrafficLook flarm_dialog;
  InfoBoxLook info_box;
  WindArrowLook wind_arrow_info_box;
  FinalGlideBarLook final_glide_bar;
  VarioBarLook vario_bar;
  IconLook icon;
  ThermalAssistantLook thermal_assistant_gauge;
  ThermalAssistantLook thermal_assistant_dialog;
  ClimbPercentLook circling_percent;

  void Initialise(const Font &map_font);
  void InitialiseConfigured(const UISettings &settings,
                            const Font &map_font, const Font &map_bold_font,
                            unsigned infobox_width);

  void ReinitialiseLayout(unsigned infobox_width, unsigned scale_title_font);
};
