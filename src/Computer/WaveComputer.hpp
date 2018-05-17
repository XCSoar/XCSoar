/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_WAVE_COMPUTER_HPP
#define XCSOAR_WAVE_COMPUTER_HPP

#include "WaveResult.hpp"
#include "StateClock.hpp"
#include "Time/DeltaTime.hpp"
#include "Math/LeastSquares.hpp"
#include "NMEA/Validity.hpp"
#include "Geo/Flat/FlatProjection.hpp"

#include <list>

struct NMEAInfo;
struct FlyingState;
struct WaveSettings;

/**
 * Detect wave locations.
 *
 * This class depends on NMEAInfo::time, NMEAInfo::location,
 * NMEAInfo::netto_vario, FlyingState::IsGliding().
 */
class WaveComputer {
  /**
   * Was the #WaveComputer enabled the last time Compute() got called?
   * This is a copy of WaveSettings::enabled.
   */
  bool last_enabled;

  DeltaTime delta_time;

  Validity last_location_available;
  Validity last_netto_vario_available;

  /**
   * Tracks for how long the glider has been sinking.  After a certain
   * duration, it is assumed that he has left the wave.
   */
  StateClock<20, 5> sinking_clock;

  FlatProjection projection;

  /**
   * The core of this algorithm: lift locations are fed into this
   * object, and it will calculate the wave axis.
   */
  LeastSquares ls;

  /**
   * List of all detected waves.  To be copied to #WaveResult.
   *
   * TODO: cleanup, prune old ones.
   */
  std::list<WaveInfo> waves;

public:
  void Reset() {
    last_enabled = false;
  }

  void Compute(const NMEAInfo &basic,
               const FlyingState &flight,
               WaveResult &result,
               const WaveSettings &settings);

private:
  void Initialise();

  /**
   * Resets the internal state for calculating the current wave.
   */
  void ResetCurrent();

  /**
   * Remove old waves.
   */
  void Decay(double min_time);

  void FoundWave(const WaveInfo &new_wave);
};

#endif
