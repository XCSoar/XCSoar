// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"
#include "Dump.hpp"
#include "Serialiser.hpp"
#include "net/ToString.hxx"

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
    cout << ToString(client.address) << '\t'
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
