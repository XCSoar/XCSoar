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

#ifndef XCSOAR_FANET_FANETADDRESS_HPP
#define XCSOAR_FANET_FANETADDRESS_HPP

#include <cstdint>
#include <cstdio>

/**
 * The identification of a FANET packets.
 */
class FanetAddress {
  uint8_t manufacturer;
  uint16_t id;

public:
  FanetAddress() = default;

  bool operator==(FanetAddress other) const {
    return id == other.id && manufacturer == other.manufacturer;
  }

  constexpr
  FanetAddress(uint8_t _manufacturer, uint16_t _id):manufacturer(_manufacturer), id(_id) {}
};

#endif // XCSOAR_FANET_FANETADDRESS_HPP
