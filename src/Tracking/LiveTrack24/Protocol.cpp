// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Protocol.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/CharUtil.hxx"
#include "util/MD5.hpp"
#include "util/NumberParser.hxx"

#include <fmt/format.h>

#include <span>

using std::string_view_literals::operator""sv;

namespace LiveTrack24 {

static MD5
Hash(std::string_view a, std::string_view b = {},
     std::string_view c = {}) noexcept
{
  MD5 md5;
  md5.Initialise();
  for (const auto s : {a, b, c})
    md5.Append(std::as_bytes(std::span{s}));
  md5.Finalize();
  return md5;
}

static std::string
HexDigest(const MD5 &md5)
{
  char buffer[MD5::DIGEST_LENGTH + 1];
  md5.GetDigest(buffer);
  return buffer;
}

/** Base64 with "-" and "_" for "+" and "/", without "=" padding. */
static std::string
Base64Url(std::span<const std::byte> src)
{
  static constexpr char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

  std::string result;
  for (std::size_t i = 0; i < src.size(); i += 3) {
    uint32_t n = uint32_t(src[i]) << 16;
    if (i + 1 < src.size())
      n |= uint32_t(src[i + 1]) << 8;
    if (i + 2 < src.size())
      n |= uint32_t(src[i + 2]);

    result += alphabet[(n >> 18) & 0x3f];
    result += alphabet[(n >> 12) & 0x3f];
    if (i + 1 < src.size())
      result += alphabet[(n >> 6) & 0x3f];
    if (i + 2 < src.size())
      result += alphabet[n & 0x3f];
  }

  return result;
}

std::string
MakeToken(std::string_view password, SessionID session,
          std::string_view app_secret)
{
  std::string lower{password};
  for (char &ch : lower)
    ch = ToLowerASCII(ch);

  const std::string password_md5 = HexDigest(Hash(lower));
  const std::string session_md5 =
    HexDigest(Hash(password_md5, fmt::format("{}", session)));
  const auto digest =
    Hash(session_md5, password_md5, app_secret).GetRawDigest();
  return Base64Url(digest);
}

std::optional<Reply>
ParseReply(std::string_view body) noexcept
{
  const auto semicolon = body.find(';');
  if (semicolon == body.npos || semicolon == 0)
    return std::nullopt;

  const auto code = ParseInteger<unsigned>(body.substr(0, semicolon));
  if (!code)
    return std::nullopt;

  body = body.substr(semicolon + 1);

  std::string_view message = body, data;
  if (const auto newline = body.find('\n'); newline != body.npos) {
    message = body.substr(0, newline);
    data = body.substr(newline + 1);
  }

  if (message.ends_with('\r'))
    message.remove_suffix(1);
  while (!data.empty() && IsWhitespaceOrNull(data.back()))
    data.remove_suffix(1);

  return Reply{*code, message, data};
}

/** Percent-encode everything except the RFC 3986 unreserved set. */
static std::string
Escape(std::string_view src)
{
  std::string result;
  for (const char ch : src) {
    if (IsAlphaNumericASCII(ch) || ch == '-' || ch == '.' || ch == '_' ||
        ch == '~')
      result += ch;
    else
      result += fmt::format("%{:02X}", (unsigned char)ch);
  }
  return result;
}

static std::string
AppPrefix(std::string_view command, const AppInfo &app)
{
  return fmt::format("/api/t/lt/{}/{}/{}/{}", command, Escape(app.key),
                     Escape(app.version), Escape(app.device_id));
}

std::string
LoginPath(const AppInfo &app, std::string_view token,
          std::string_view username)
{
  return fmt::format("{}/0/0/{}/{}", AppPrefix("getUserID"sv, app),
                     token, Escape(username));
}

/* public track (privacy 0), vehicle described by the app (profile 99) */
static constexpr unsigned PRIVACY = 0, PROFILE = 99;

static std::string
SessionPrefix(std::string_view command, const AppInfo &app,
              const Session &session, unsigned packet_id)
{
  return fmt::format("{}/{}/{}/{}/{}/{}/{}", AppPrefix(command, app),
                     session.id, session.user, session.token,
                     PRIVACY, PROFILE, packet_id);
}

std::string
TrackPath(const AppInfo &app, const Session &session, unsigned packet_id,
          const Fix &fix, const TrackInfo *info)
{
  const auto time = std::chrono::system_clock::to_time_t(fix.time);
  std::string path = fmt::format("{}/{}/{:.6f}/{:.6f}/{}/{}/{}",
                                 SessionPrefix("track"sv, app, session,
                                               packet_id),
                                 time,
                                 fix.location.latitude.Degrees(),
                                 fix.location.longitude.Degrees(),
                                 fix.altitude, fix.speed, fix.course);

  if (info != nullptr)
    path += fmt::format("/{}:{}:{}", unsigned(info->category),
                        Escape(info->vehicle_name),
                        Escape(info->device_model));

  return path;
}

std::string
TrackEndPath(const AppInfo &app, const Session &session,
             unsigned packet_id)
{
  /* status 0 = everything OK, 1 = submit the track */
  return SessionPrefix("trackEnd"sv, app, session, packet_id) + "/0/1";
}

} // namespace LiveTrack24
