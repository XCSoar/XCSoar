// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermIndexJson.hpp"

#include <stdexcept>

namespace boost::json {

static unsigned
ToUint(const value &v, unsigned default_val) noexcept
{
  if (v.is_uint64())
    return (unsigned)v.as_uint64();

  if (v.is_int64()) {
    const auto n = v.as_int64();
    return n >= 0 ? (unsigned)n : default_val;
  }

  return default_val;
}

XCThermAPI::ForecastSlot
tag_invoke(value_to_tag<XCThermAPI::ForecastSlot>, const value &jv)
{
  if (!jv.is_object())
    throw std::runtime_error("Invalid slot object");

  const auto &obj = jv.as_object();
  XCThermAPI::ForecastSlot slot;

  const std::string run_str = value_to<std::string>(obj.at("run"));
  if (run_str.size() < 13)
    throw std::runtime_error("Invalid run datetime");

  slot.run_date = run_str.substr(0, 4) +
                  run_str.substr(5, 2) +
                  run_str.substr(8, 2);
  slot.run_hour = run_str.substr(11, 2);

  const auto *steps = obj.if_contains("steps");
  if (steps != nullptr && steps->is_object()) {
    const auto &steps_obj = steps->as_object();
    if (const auto *v = steps_obj.if_contains("min"))
      slot.step_min = ToUint(*v, 0);
    if (const auto *v = steps_obj.if_contains("max"))
      slot.step_max = ToUint(*v, 0);
    if (const auto *v = steps_obj.if_contains("step"))
      slot.step_step = ToUint(*v, 1);
    if (slot.step_step == 0)
      slot.step_step = 1;
  }

  return slot;
}

} // namespace boost::json
