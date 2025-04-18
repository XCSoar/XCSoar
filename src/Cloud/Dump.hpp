// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/AllocatedSocketAddress.hxx"
#include "net/ToString.hxx"

#include <iostream>
#include <iomanip>

inline std::ostream &
operator<<(std::ostream &stream, SocketAddress address)
{
  return stream << ToString(address);
}

inline std::ostream &
operator<<(std::ostream &stream, const AllocatedSocketAddress &address)
{
  return stream << SocketAddress(address);
}

template<char positive, char negative>
struct GeoAngle : Angle {
  constexpr GeoAngle(Angle _angle):Angle(_angle) {}
};

template<char positive, char negative>
inline std::ostream &
operator<<(std::ostream &stream, const GeoAngle<positive, negative> value)
{
  const auto dms = value.ToDMS();
  return stream << std::setfill('0')
                << dms.degrees << '.'
                << std::setw(2) << dms.minutes << '.'
                << std::setw(2) << dms.seconds
                << (dms.negative ? negative : positive)
                << std::setfill(' ');
}

inline std::ostream &
operator<<(std::ostream &stream, const GeoPoint &p)
{
  return p.IsValid()
    ? stream << GeoAngle<'N', 'S'>(p.latitude)
             << '/'
             << GeoAngle<'E', 'W'>(p.longitude)
    : stream << '?';
}
