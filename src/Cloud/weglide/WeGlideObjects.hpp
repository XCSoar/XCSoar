#pragma once

#include "util/StaticString.hxx"
#include "time/BrokenDate.hpp"

#include <cinttypes>
#include <tchar.h>


namespace WeGlide {

struct User {
  uint32_t id;
  BrokenDate birthdate;
  StaticString<0x80> name;

  constexpr bool IsValid() const noexcept
  {
    return id > 0;
  }

  void Clear() noexcept
  {
    id = 0;
    name.clear();
    birthdate.Clear();
  }
};
// because it is used in Settings and there it has to be 'trivial' type!
  static_assert(std::is_trivial<User>::value, "type is not trivial");

struct Aircraft {
  uint32_t id = 0;
  StaticString<0x40> name;
  StaticString<4> kind; // 'MG' - motor aircraft, GL - Glider...
  StaticString<10> sc_class;

  constexpr bool IsValid() const noexcept
  {
    return id > 0;
  }
  void Clear() noexcept {
    id = 0;
    name.clear();
    kind.clear();
    sc_class.clear();
  }
};

struct Flight {
  uint64_t flight_id = 0;
  User user;
  Aircraft aircraft;
  StaticString<0x40> igc_name;
  StaticString<0x40> scoring_date;
  StaticString<0x40> registration;
  StaticString<0x40> competition_id;

  constexpr bool IsValid() const noexcept
  {
    return flight_id > 0;
  }
};

};