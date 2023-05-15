// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Error.hpp"
#include "lib/fmt/RuntimeError.hxx"

#include <boost/json/value.hpp>

using std::string_view_literals::operator""sv;

namespace WeGlide {

std::runtime_error
ResponseToException(unsigned status, const boost::json::value &body)
{
  const auto &o = body.as_object();

  if (status == 422) {
    // validation error

    if (auto d = o.find("detail"sv); d != o.end()) {
      const auto &detail = d->value().as_array().at(0).as_object();
      if (auto i = detail.find("msg"sv); i != detail.end())
        return FmtRuntimeError("WeGlide status {}: {}",
                               status,
                               (std::string_view)i->value().as_string());
    }
  }

  if (auto i = o.find("error_description"sv); i != o.end())
    return FmtRuntimeError("WeGlide status {}: {}",
                           status,
                           (std::string_view)i->value().as_string());

  if (auto i = o.find("error"sv); i != o.end())
    return FmtRuntimeError("WeGlide status {}: {}",
                           status,
                           (std::string_view)i->value().as_string());

  return FmtRuntimeError("WeGlide status {}", status);
}

} // namespace WeGlide
