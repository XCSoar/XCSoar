// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Places.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/ATCReference.hpp"
#include "InfoBoxes/Panel/ATCSetup.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Formatter/Units.hpp"

void
UpdateInfoBoxHomeDistance(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;

  if (!common_stats.vector_home.IsValid()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(common_stats.vector_home.distance);

  if (basic.track_available) {
    Angle bd = common_stats.vector_home.bearing - basic.track;
    data.SetCommentFromBearingDifference(bd);
  } else
    data.SetCommentInvalid();
}

void
UpdateInfoBoxTakeoffDistance(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const FlyingState &flight = CommonInterface::Calculated().flight;

  if (!basic.location_available || !flight.flying ||
      !flight.takeoff_location.IsValid()) {
    data.SetInvalid();
    return;
  }

  const GeoVector vector(basic.location, flight.takeoff_location);
  data.SetValueFromDistance(vector.distance);

  if (basic.track_available)
    data.SetCommentFromBearingDifference(vector.bearing - basic.track);
  else
    data.SetCommentInvalid();
}

#ifdef __clang__
/* gcc gives "redeclaration differs in 'constexpr'" */
constexpr
#endif
const InfoBoxPanel atc_infobox_panels[] = {
  { N_("Reference"), LoadATCReferencePanel },
  { N_("Setup"), LoadATCSetupPanel },
  { nullptr, nullptr }
};

void
UpdateInfoBoxATCRadial(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const GeoPoint &reference =
    CommonInterface::GetComputerSettings().poi.atc_reference;
  const Angle &declination =
    CommonInterface::GetComputerSettings().poi.magnetic_declination;

  if (!basic.location_available || !reference.IsValid()) {
    data.SetInvalid();
    return;
  }

  const GeoVector vector(reference, basic.location);

  const Angle mag_bearing = vector.bearing - declination;
  data.SetValue(mag_bearing);

  FormatDistance(data.comment.buffer(), vector.distance,
                 Unit::NAUTICAL_MILES, true, 1);
}
