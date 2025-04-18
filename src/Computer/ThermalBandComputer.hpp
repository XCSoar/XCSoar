// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"

struct MoreData;
struct DerivedInfo;
class ThermalEncounterBand;
class ThermalEncounterCollection;

/**
 * Record the thermal band.
 *
 * @see ThermalBandInfo
 */
class ThermalBandComputer {
  Validity last_vario_available;

public:
  void Reset();

  void Compute(const MoreData &basic, const DerivedInfo &calculated,
               ThermalEncounterBand &tbe,
               ThermalEncounterCollection &tbc);
private:
  bool in_encounter;
};
