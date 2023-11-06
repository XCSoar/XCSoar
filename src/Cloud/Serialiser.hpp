// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/BufferedOutputStream.hxx"
#include "io/BufferedReader.hxx"
#include "util/ByteOrder.hxx"
#include "util/SpanCast.hxx"

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
    Write(ReferenceAsBytes(value));
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

  void WriteString(std::string_view s);

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
    ReadFullT(value);
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
