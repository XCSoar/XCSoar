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
  { 0, "Auto" },
  { 1, "4800" },
  { 2, "9600" },
  { 3, "19200" },
  { 4, "38400" },
  { 5, "57600" },
  { 6, "115200" },
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
    N_("Whether the internal accelerometer is used. Only change this if the accelerometer has malfunctioned or the instrument cannot be installed with correct alignment."),
  },
  { DataField::Type::ENUM, "HasTemperature",
    N_("Temperature"),
    N_("Whether a temperature and humidity sensor is installed. Set to 0 to disable, 255 to enable auto-detect; otherwise the 1-Wire device ID can be specified."),
    tri_state,
  },
  { DataField::Type::ENUM, "BaudrateA",
    N_("Baud rate Vega"),
    N_("Baud rate of serial device connected to Vega port X1. Use this when using a third party GPS or data-logger instead of FLARM. If FLARM is connected the baud rate will be fixed at 38400. For OzFLARM, the value can be set to 19200."),
    baud_rates,
  },
  { DataField::Type::BOOLEAN, "FlarmConnected",
    N_("FLARM connected"),
    N_("Enable detection of FLARM. Disable only if FLARM is not used or disconnected."),
  },
  { DataField::Type::BOOLEAN, "EnablePDASupply",
    N_("PDA power"),
    N_("Enable output of the +5 V power supply for a PDA or similar device at Vega connector X2. If Vega is connected to Altair, set this to Off."),
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
