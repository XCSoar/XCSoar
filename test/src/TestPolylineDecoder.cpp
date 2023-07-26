// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/PolylineDecoder.hpp"
#include "util/PrintException.hxx"
#include "TestUtil.hpp"

#include <tchar.h>

using std::string_view_literals::operator""sv;

static constexpr GeoPoint
GP(double latitude, double longitude) noexcept
{
  return {Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

int main()
try {
  /* this is the sample from
     https://developers.google.com/maps/documentation/utilities/polylinealgorithm */
  static constexpr auto encoded = "_p~iF~ps|U_ulLnnqC_mqNvxq`@";
  static constexpr auto decoded = std::array{
    GP(38.5, -120.2),
    GP(40.7, -120.95),
    GP(43.252, -126.453),
  };

  plan_tests(1 + decoded.size());

  const auto v = DecodePolyline(encoded);
  ok1(v.size() == decoded.size());

  for (std::size_t i = 0; i < decoded.size(); ++i) {
    ok1(equals(v[i], decoded[i]));
  }

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
