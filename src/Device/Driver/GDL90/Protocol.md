# GDL90 protocol (XCSoar driver)

This note describes what the **GDL90** device driver implements. Authoritative
sources:

- **Garmin GDL 90 Data Interface Specification** (560-1058-00 Rev A, 2007),
  public ICD (e.g. FAA archival PDF).
- **ForeFlight GDL 90 Extended Specification**
  (<https://foreflight.com/connect/spec/>) for message id `0x65`.

## Serial framing (ICD §2.2)

- Frames are delimited by flag byte **`0x7E`**.
- Between flags, **`0x7D`** is an escape: the next byte is XORed with **`0x20`**
  to recover the real value (so literal `0x7E` / `0x7D` can appear in payload).
- After unescaping, a logical message is:
  **`[message_id][payload…][crc_lo][crc_hi]`**
- **CRC-16-CCITT** (polynomial as in ICD §2.2.3) is computed over
  `message_id` + `payload`. The CRC is stored **little-endian** (low byte
  first on the wire).
- The **top bit** of `message_id` is reserved and must be zero (ICD §2.2.2).

## Endianness

- **Payload fields** in standard messages are **big-endian** (MSB first), unless
  the ICD says otherwise.
- **CRC** is **little-endian**.
- **Heartbeat** time-of-day uses a **little-endian** 16-bit second count
  (plus one high bit in a status byte); see below.

## Message id `0x00` — Heartbeat (ICD §3.1)

**Payload** (6 bytes minimum after stripping `message_id`):

| Offset | Content |
|--------|---------|
| 0 | Status byte 1 |
| 1 | Status byte 2 (Stratux: bit 0 = UTC time valid; bit 7 = second count bit 16) |
| 2–3 | Seconds since midnight UTC, **little-endian**, bits 0–15 |
| 4–5 | Uplink / basic message counts (ignored by XCSoar for time) |



## Message ids `0x0A` / `0x14` — Ownship / Traffic (ICD §3.4 / §3.5, body §3.5.1)

Both use the same **27-byte** payload layout (**indices after `message_id`**):

| Offset | Size | Field |
|--------|------|--------|
| 0 | 1 | Status: alert (low nibble), address type (high nibble) |
| 1–3 | 3 | Participant address (24-bit big-endian) |
| 4–6 | 3 | Latitude: signed 24-bit semicircles (ICD §3.5.1.3) |
| 7–9 | 3 | Longitude: same encoding |
| 10–11 | 2 | Altitude word: 12-bit pressure altitude code (25 ft LSB, −1000 ft offset), high nibble of word; low 4 bits misc / track validity |
| 12 | 1 | NACp / NIC supplement |
| 13–15 | 3 | Velocity: upper 12 bits ground speed (kt), lower 12 bits vertical rate (64 fpm LSB, sign-extended 12-bit); `0xFFF` / `0x800` = invalid / N/A per ICD |
| 16 | 1 | Track heading: `floor(track_deg / 360 * 256)`; `0xFF` = invalid |
| 17 | 1 | Emitter category (ICD §3.5.1.10) |
| 18–25 | 8 | Callsign ASCII (space-padded) |
| 26 | 1 | Emergency / priority (not interpreted in basic path) |

Ownship handling uses position, altitude word, horizontal speed, and track;
traffic handling fills **FLARM** traffic slots from the same offsets.

**Address type (high nibble of byte 0)** maps to `FlarmTraffic::source` for
traffic reports: ADS-B (0, 1), TIS-B (2, 3, 4 surface vehicle), ADS-R (6),
other values default to ADS-B. Details: ICD §3.5.1.2; some receivers place the
qualifier in the low nibble instead—those targets may be misclassified until
the sender follows the ICD layout (`tools/gdl90_send.py` uses the high nibble).

## Message id `0x0B` — Ownship geometric altitude (ICD §3.6)

**Payload:** signed altitude in **5 ft** steps as **big-endian int16** at
offset **0**, then **2 bytes** vertical metrics (VFOM etc.). XCSoar reads the
first **4** bytes; altitude datum (ellipsoid vs MSL) can follow ForeFlight’s
ID message capability bit (see below).

## Message id `0x65` — ForeFlight extensions

Sub-id is **first payload byte** after `0x65`:

### Sub-id `0x00` — ID message

- Byte 1: version (must be **1** for supported layout).
- Bytes 2–33: serial, short name, long name (see ForeFlight spec).
- Bytes **34–37**: **capabilities mask**, **big-endian** 32-bit.
  - **Bit 0:** if set, ownship geometric altitude (`0x0B`) is **MSL**; if clear,
    **WGS-84 ellipsoid** (default GDL90 meaning).

### Sub-id `0x01` — AHRS

All multi-byte numeric fields **big-endian**, 0.1° for attitude/heading where
applicable; airspeeds in knots; `0x7FFF` / `0xFFFF` = invalid per field (see
ForeFlight spec table).

## Tools and tests

- `tools/gdl90_send.py` builds sample frames for local testing.
- `test/src/TestGDL90.cpp` and `test/src/TestGDL90Driver.cpp` exercise CRC,
  unescape, and driver behaviour.
