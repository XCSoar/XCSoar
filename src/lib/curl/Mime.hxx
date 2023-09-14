// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <curl/curl.h>

#include <utility>

/**
 * An OO wrapper for a "curl_mime" object.
 */
class CurlMime {
	curl_mime *mime;

public:
	explicit CurlMime(CURL *easy) noexcept
		:mime(curl_mime_init(easy)) {}

	CurlMime(CurlMime &&src) noexcept
		:mime(std::exchange(src.mime, nullptr)) {}

	~CurlMime() noexcept {
		if (mime != nullptr)
			curl_mime_free(mime);
	}

	auto &operator=(CurlMime &&src) noexcept {
		using std::swap;
		swap(mime, src.mime);
		return *this;
	}

	curl_mime *get() const noexcept {
		return mime;
	}

	class Part {
		curl_mimepart *part;

	public:
		constexpr Part(curl_mimepart *_part) noexcept
			:part(_part) {}

		auto Name(const char *name) noexcept {
			curl_mime_name(part, name);
			return *this;
		}

		auto Data(const void *data, std::size_t size) noexcept {
			curl_mime_data(part, (const char *)data, size);
			return *this;
		}

		auto Data(const char *data) noexcept {
			curl_mime_data(part, data, CURL_ZERO_TERMINATED);
			return *this;
		}

		auto FileData(const char *path) noexcept {
			curl_mime_filedata(part, path);
			return *this;
		}

		auto Filename(const char *name) noexcept {
			curl_mime_filename(part, name);
			return *this;
		}

		auto Type(const char *type) noexcept {
			curl_mime_type(part, type);
			return *this;
		}
	};

	Part Add() const noexcept {
		return curl_mime_addpart(mime);
	}

	Part Add(const char *name) const noexcept {
		return Add().Name(name);
	}
};
