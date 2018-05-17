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

#include "Data.hpp"
#include "Dump.hpp"
#include "Serialiser.hpp"

#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;

static constexpr uint32_t CLOUD_MAGIC = 0x5753f60f;
static constexpr uint32_t CLOUD_VERSION = 1;

void
CloudData::DumpClients()
{
  for (const auto &client : clients) {
    cout << client.endpoint << '\t'
         << std::hex << client.key << std::dec << '\t'
         << client.id << '\t'
         << client.location << '\t'
         << client.altitude << "m\n";
  }

  cout.flush();
}

void
CloudData::Save(Serialiser &s) const
{
  s.Write32(CLOUD_MAGIC);
  s.Write32(CLOUD_VERSION);
  clients.Save(s);
  s.Write8(1);
  thermals.Save(s);
  s.Write8(0);
}

void
CloudData::Load(Deserialiser &s)
{
  if (s.Read32() != CLOUD_MAGIC)
    throw std::runtime_error("Bad magic");

  if (s.Read32() != CLOUD_VERSION)
    throw std::runtime_error("Bad version");

  clients.Load(s);

  if (s.Read8() != 0) {
    thermals.Load(s);
    s.Read8();
  }
}
