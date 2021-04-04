/*
 * Copyright 2011-2021 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Multi.hxx"

#include <cassert>

CurlMulti::~CurlMulti()
{
	assert(results.empty());

	if (handle != nullptr)
		curl_multi_cleanup(handle);
}

void
CurlMulti::Remove(CURL *easy)
{
	auto i = results.find(easy);
	if (i != results.end())
		results.erase(i);

	curl_multi_remove_handle(handle, easy);
}

CURLcode
CurlMulti::InfoRead(const CURL *easy)
{
	auto i = results.find(easy);
	if (i != results.end())
		return i->second;

	while (const CURLMsg *msg = InfoRead()) {
		if (msg->msg == CURLMSG_DONE) {
			if (msg->easy_handle == easy)
				return msg->data.result;

			results.insert(std::make_pair(easy, msg->data.result));
		}
	}

	return CURLE_AGAIN;
}
