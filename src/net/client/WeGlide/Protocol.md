# WeGlide protocol (XCSoar client)

How XCSoar talks to WeGlide for Copilot map layers. Feature request:
[issue 1403](https://github.com/XCSoar/XCSoar/issues/1403).

Authoritative API description:
[https://api.weglide.org/docs](https://api.weglide.org/docs)
(OpenAPI at `/v1/openapi.json`). The Copilot tile endpoints are only
sketched there; this note records the fields XCSoar relies on.

Auth and the tile-token call use `https://api.weglide.org`. Tile URLs
use the `base_url` returned with the tile token.

A profile API key (`X-API-Key`, 60 requests per day) is a different
credential. It reads public data such as `GET /v1/aircraft/simple/{id}`.
`GET /v1/copilot/tiles/token` still requires the device-flow bearer
token and answers 401 `missing_authorization` when only the API key is
sent. Copilot does not use that key.

The WAF rejects curl’s default User-Agent. XCSoar’s own User-Agent is
allowed. To try a request:

```bash
curl -A "XCSoar/7.45" …
```

Form bodies below are `application/x-www-form-urlencoded`, as in
[RFC 8628](https://www.rfc-editor.org/rfc/rfc8628).

## Lifetimes

| What | How often | Validity |
|------|-----------|----------|
| Device login (device flow) | once per device | Access token 10 days, then refresh |
| Tile token | about every 90 min | 2 hours |
| Layer `time` | smaller `interval` (20 s / 60 s observed) | `interval` from `latest` |
| PNG tile | cache without expiry | A URL’s content never changes |

## 1. Login (once per device)

[RFC 8628](https://www.rfc-editor.org/rfc/rfc8628) device authorization
grant. `client_id` is fixed: `xcsoar`. Scope (space-separated):
`upload declare copilot:layers`.

### 1.1 Request a device code

`POST /v1/auth/device/authorize`

```
client_id=xcsoar&scope=upload declare copilot:layers
```

Response:

```json
{
  "device_code": "…",
  "user_code": "…",
  "verification_uri": "…",
  "verification_uri_complete": "…",
  "expires_in": 1800,
  "interval": 5
}
```

`expires_in` is 1800 s (30 min). `interval` is the wait between token
polls, here 5 s.

Show the pilot `user_code` and `verification_uri`, or
`verification_uri_complete` as a QR code. Confirmation happens on the
phone.

### 1.2 Wait for confirmation

Every `interval` seconds, until success, expiry, or cancellation:

`POST /v1/auth/token`

```
grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=…&client_id=xcsoar
```

Until the pilot confirms:

```
400 {"error":"authorization_pending"}
```

Other `error` values:

| `error` | Meaning |
|---------|---------|
| `authorization_pending` | keep polling at `interval` |
| `slow_down` | increase the gap, then keep polling |
| `expired_token` | `device_code` expired (`expires_in`); start the flow again |
| `access_denied` | the pilot denied the request |

Success:

```json
{
  "token_type": "Bearer",
  "access_token": "…",
  "expires_in": 864000,
  "refresh_token": "…",
  "scope": "…"
}
```

`expires_in` of 864000 s is 10 days. Every later call to
`api.weglide.org` sends `Authorization: Bearer <access_token>`.

`/v1/auth/token` is limited to 100 calls per 5 minutes per IP. Honour
the reported `interval`.

### 1.3 Refresh the access token

Before it expires:

`POST /v1/auth/token`

```
grant_type=refresh_token&refresh_token=…&client_id=xcsoar
```

The response contains a new refresh token. The previous one is then
invalid; store only the new one.

### 1.4 What the same token also allows

- `POST /v1/igcfile` without `user_id` and without `date_of_birth`
- `GET /v1/task`, including private tasks

## 2. Tile token (about every 90 minutes)

`GET /v1/copilot/tiles/token` with the bearer token.

```json
{
  "query": "token=HS256-…&expires=…&g=…&token_path=%2Fapi%2Fgrid%2F",
  "expires": 1792415012,
  "base_url": "https://copilot-tiles.weglide.org/api"
}
```

`expires` is a Unix timestamp. The token is valid for 2 hours. Tile
URLs are `{base_url}/grid/…`. Append `query` unchanged. Without a
valid `query`, the CDN returns 403. Extra query parameters also
return 403, so do not add anything around `query`.

Fetch a new one when less than 30 minutes of lifetime remain, or once
after a 403 or a connection error. Without a subscription the response
is 403; then wait 10 minutes before the next attempt.

## 3. Layers and tiles

### Catalog

`GET {base_url}/grid/catalog?{query}`

```json
[{
  "id": "thermal",
  "name": "Cloudbase",
  "description": "Height reached in the thermals of the hour before the tick",
  "units": "m",
  "legend": { "colors": [{ "value": 350.0, "color": [0, 0, 143] }] }
}]
```

Two layers. `thermal` is cloudbase, units `m`, values about
350–4100. `vario` is netto, units `m/s`, values about −0.7–4.3.
`color` is `[r, g, b]`.

Adding `lon` and `lat` makes the CDN return 403. Checked on
2026-10-19 with the query both before and after those parameters.
Call catalog with `query` alone until that changes.

### Current time

`GET {base_url}/grid/latest?{query}`

```json
[
  { "layer_id": "thermal", "time": 1792407840, "interval": 60, "max_zoom": 7 },
  { "layer_id": "vario", "time": 1792407840, "interval": 20, "max_zoom": 7 }
]
```

`time` is a Unix timestamp and `interval` is the frame step, in
seconds. On 2026-10-19 thermal was 60 s and vario was 20 s, and both
rows stayed on `1792407840` while the clock moved hours ahead. Tile
URLs at `time + n * interval` (the newest step that is still not
after now) returned the painted PNGs; the origin time itself was
almost empty. Do not invent a time when `time` or `interval` is
missing or zero. Refetch `latest` at the smaller interval.

### Tile

`GET {base_url}/grid/{layer_id}/{time}/{z}/{x}/{y}.png?{query}`

256 px PNG with alpha, slippy map / Web Mercator, up to `max_zoom` 7.

The content of a given URL never changes. Cache each `layer_id` /
`time` / `z` / `x` / `y` locally with no expiry. A new tick from
`latest` produces new URLs.
