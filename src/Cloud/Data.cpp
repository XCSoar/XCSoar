// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"
#include "Dump.hpp"
#include "Serialiser.hpp"
#include "net/ToString.hxx"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cstdio>

static constexpr uint32_t CLOUD_MAGIC = 0x5753f60f;
static constexpr uint32_t CLOUD_VERSION = 1;

void
CloudData::DumpClients()
{
  for (const auto &client : clients) {
    fmt::print(stdout,
               "{}\t{:x}\t{}\t{}\t{}m\n",
               ToString(client.address),
               client.key,
               client.id,
               fmt::streamed(client.location),
               client.altitude);
    fflush(stdout);
  }
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
