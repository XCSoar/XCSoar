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

#ifndef XCSOAR_SERIALISER_HPP
#define XCSOAR_SERIALISER_HPP

#include "IO/BufferedOutputStream.hxx"
#include "IO/BufferedReader.hxx"
#include "OS/ByteOrder.hpp"

#include <string>
#include <chrono>

#include <stdio.h>

class Serialiser : public BufferedOutputStream {
  const std::chrono::steady_clock::time_point steady_now =
    std::chrono::steady_clock::now();
  const std::chrono::system_clock::time_point system_now =
    std::chrono::system_clock::now();

public:
  explicit Serialiser(OutputStream &_os):BufferedOutputStream(_os) {}

  template<typename T>
  void WriteT(const T &value) {
    Write(&value, sizeof(value));
  }

  void Write8(uint8_t value) {
    WriteT(value);
  }

  void Write16(uint16_t value) {
    value = ToBE16(value);
    WriteT(value);
  }

  void Write32(uint32_t value) {
    value = ToBE32(value);
    WriteT(value);
  }

  void Write64(uint64_t value) {
    value = ToBE64(value);
    WriteT(value);
  }

  void WriteFloat(float value) {
    union {
      float f;
      uint32_t i;
    } u;
    static_assert(sizeof(u.f) == sizeof(u.i), "");

    u.f = value;
    Write32(u.i);
  }

  void WriteDouble(double value) {
    union {
      double f;
      uint64_t i;
    } u;
    static_assert(sizeof(u.f) == sizeof(u.i), "");

    u.f = value;
    Write64(u.i);
  }

  void WriteString(const char *s);

  void WriteString(const std::string &s) {
    WriteString(s.c_str());
  }

  Serialiser &operator<<(std::chrono::system_clock::time_point t) {
    Write64(std::chrono::system_clock::to_time_t(t));
    return *this;
  }

  Serialiser &operator<<(std::chrono::steady_clock::time_point t) {
    const auto _delta = steady_now - t;
    const auto delta = std::chrono::duration_cast<std::chrono::system_clock::duration>(_delta);
    const auto u = system_now - delta;
    return *this << u;
  }
};

class Deserialiser : public BufferedReader {
  const std::chrono::steady_clock::time_point steady_now =
    std::chrono::steady_clock::now();
  const std::chrono::system_clock::time_point system_now =
    std::chrono::system_clock::now();

public:
  explicit Deserialiser(Reader &_r):BufferedReader(_r) {}

  using BufferedReader::Read;

  template<typename T>
  void ReadT(T &value) {
    ReadFull({&value, sizeof(value)});
  }

  uint8_t Read8() {
    uint8_t value;
    ReadT(value);
    return value;
  }

  uint16_t Read16() {
    uint16_t value;
    ReadT(value);
    return FromBE16(value);
  }

  uint32_t Read32() {
    uint32_t value;
    ReadT(value);
    return FromBE32(value);
  }

  uint64_t Read64() {
    uint64_t value;
    ReadT(value);
    return FromBE64(value);
  }

  float ReadFloat() {
    union {
      float f;
      uint32_t i;
    } u;
    static_assert(sizeof(u.f) == sizeof(u.i), "");

    u.i = Read32();
    return u.f;
  }

  double ReadDouble() {
    union {
      double f;
      uint64_t i;
    } u;
    static_assert(sizeof(u.f) == sizeof(u.i), "");

    u.i = Read64();
    return u.f;
  }

  std::string ReadString();

  Deserialiser &operator>>(std::chrono::system_clock::time_point &t) {
    t = std::chrono::system_clock::from_time_t(Read64());
    return *this;
  }

  Deserialiser &operator>>(std::chrono::steady_clock::time_point &t) {
    std::chrono::system_clock::time_point u;
    *this >> u;

    const auto delta = system_now - u;
    t = steady_now - delta;
    return *this;
  }
};

#endif
