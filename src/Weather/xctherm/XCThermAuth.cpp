// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAuth.hpp"
#include "LogFile.hpp"

#include <cstdint>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

#include <curl/curl.h>

static size_t
AuthWriteCallback(void *contents, size_t size, size_t nmemb,
                  std::vector<uint8_t> *buffer) {
  const size_t total = size * nmemb;
  auto *ptr = static_cast<uint8_t *>(contents);
  buffer->insert(buffer->end(), ptr, ptr + total);
  return total;
}

static void
AddResolveEntry(curl_slist *&list, const char *host, const char *ip) {
  const std::string entry = std::string(host) + ":443:" + ip;
  list = curl_slist_append(list, entry.c_str());
}

XCThermAuthResult
XCThermAuthenticate(const std::string &email,
                    const std::string &password) noexcept {
  XCThermAuthResult result;

  if (email.empty() || password.empty()) {
    result.error_message = "Email or password empty";
    return result;
  }

  LogFmt("xctherm: authenticate user='{}'", email);

  const std::string post_data =
    R"({"email":")" + email + R"(","password":")" + password + R"("})";

  static constexpr const char *auth_hosts[] = {
    "https://xctherm.com",
    "https://www.xctherm.com",
  };

  std::vector<uint8_t> response_buffer;
  CURLcode curl_result = CURLE_OK;
  long http_code = 0;

  static constexpr int kMaxAttempts = 2;
  bool auth_ok = false;

  for (int attempt = 1; attempt <= kMaxAttempts && !auth_ok; ++attempt) {
    for (const char *auth_host : auth_hosts) {
      response_buffer.clear();
      http_code = 0;

      const std::string url =
        std::string(auth_host) + "/api/accounts/authenticate";

      CURL *curl = curl_easy_init();
      if (!curl)
        continue;

      struct curl_slist *headers = nullptr;
      struct curl_slist *resolve_list = nullptr;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      AddResolveEntry(resolve_list, "xctherm.com", "138.201.15.242");
      AddResolveEntry(resolve_list, "www.xctherm.com", "138.201.15.242");

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AuthWriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

      curl_result = curl_easy_perform(curl);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

      curl_slist_free_all(headers);
      curl_slist_free_all(resolve_list);
      curl_easy_cleanup(curl);

      if (curl_result == CURLE_OK && http_code == 200) {
        auth_ok = true;
        break;
      }

      LogFmt("xctherm: auth attempt {}/{} host={} curl={} http={}",
             attempt, kMaxAttempts, auth_host,
             (int)curl_result, http_code);
    }

    if (!auth_ok && attempt < kMaxAttempts)
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  if (!auth_ok) {
    result.error_message = "Login failed (HTTP " +
      std::to_string(http_code) + ")";
    LogFmt("xctherm: auth failed curl={} http={}",
           (int)curl_result, http_code);
    return result;
  }

  /* Parse JWT from JSON response */
  std::string response(response_buffer.begin(), response_buffer.end());

  size_t pos = response.find("\"jwtToken\":\"");
  if (pos == std::string::npos) {
    result.error_message = "No token in server response";
    return result;
  }

  pos += 12; // skip "jwtToken":"
  size_t end_pos = response.find("\"", pos);
  if (end_pos == std::string::npos) {
    result.error_message = "Malformed token in response";
    return result;
  }

  result.jwt_token = response.substr(pos, end_pos - pos);
  result.success = true;

  LogFmt("xctherm: auth success, token_len={}", result.jwt_token.size());
  return result;
}
