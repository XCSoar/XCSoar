// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice tri_state[] = {
  { 0, N_("Off") },
  { 1, N_("On") },
  { 255, N_("Auto") },
  { 0 },
};

static constexpr StaticEnumChoice baud_rates[] = {
  { 0, _T("Auto") },
  { 1, _T("4800") },
  { 2, _T("9600") },
  { 3, _T("19200") },
  { 4, _T("38400") },
  { 5, _T("57600") },
  { 6, _T("115200") },
  { 0 },
};

static constexpr
VegaParametersWidget::StaticParameter hardware_parameters[] = {
  { DataField::Type::BOOLEAN, "HasPressureTE",
    N_("TE port"), N_("Whether the Total Energy port is connected."),
  },
  { DataField::Type::BOOLEAN, "HasPressurePitot",
    N_("Pitot port"), N_("Whether the Pitot port is connected."),
  },
  { DataField::Type::BOOLEAN, "HasPressureStatic",
    N_("Static port"), N_("Whether the Static port is connected."),
  },
  { DataField::Type::BOOLEAN, "HasPressureStall",
    N_("Stall port"), N_("Whether the Stall pressure port is connected."),
  },
  { DataField::Type::BOOLEAN, "HasAccelerometer",
    N_("Accelerometer"),
    N_("Whether the internal accelerometer is used.  Only change this if the accelerometer has malfunctioned or the instrument cannot be installed with correct alignment."),
  },
  { DataField::Type::ENUM, "HasTemperature",
    N_("Temperature"),
    N_("Whether a temperature and humidity sensor is installed.  Set to 0 to disable, 255 to enable auto-detect; otherwise the 1Wire device ID can be specified."),
    tri_state,
  },
  { DataField::Type::ENUM, "BaudrateA",
    N_("Baud rate Vega"),
    N_("Baud rate of serial device connected to Vega port X1.  Use this as necessary when using a third party GPS or data-logger instead of FLARM.  If FLARM is connected the baud rate will be fixed at 38400.  For OzFLARM, the value can be set to 19200."),
    baud_rates,
  },
  { DataField::Type::BOOLEAN, "FlarmConnected",
    N_("FLARM connected"),
    N_("Enable detection of FLARM.  Disable only if FLARM is not used or disconnected."),
  },
  { DataField::Type::BOOLEAN, "EnablePDASupply",
    N_("PDA power"),
    N_("Enable output of +5V power supply for PDA etc. at Vega connector~X2.  If Vega is connected to Altair, this should be set to False."),
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
