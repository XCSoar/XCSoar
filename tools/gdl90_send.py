#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Generate synthetic GDL90 traffic and send via UDP.
#
# Usage example:
#   python3 tools/gdl90_send.py --host 127.0.0.1 --port 4352 --n-targets 8
#
# Configure XCSoar device:
#   Driver: GDL90
#   Port: UDP listener on the same port (e.g. 4352)

from __future__ import annotations

import argparse
import math
import random
import socket
import struct
import time
from datetime import datetime, timezone
from dataclasses import dataclass
from typing import Iterable, Optional


GDL90_FLAG = 0x7E
GDL90_ESCAPE = 0x7D
GDL90_ESCAPE_XOR = 0x20


def _crc16_ccitt_table() -> list[int]:
    """
    CRC-CCITT table (poly 0x1021), matching XCSoar's CRC16-CCITT.
    """
    table: list[int] = []
    for i in range(256):
        crc = i << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
            crc &= 0xFFFF
        table.append(crc)
    return table


_CRC_TABLE = _crc16_ccitt_table()


def gdl90_crc16(data: bytes) -> int:
    """
    Table-driven CRC update in the style used by XCSoar's driver/tests:
      crc = table[crc >> 8] ^ (crc << 8) ^ b
    """
    crc = 0
    for b in data:
        crc = _CRC_TABLE[(crc >> 8) & 0xFF] ^ ((crc << 8) & 0xFFFF) ^ b
    return crc & 0xFFFF


def gdl90_escape(body: bytes) -> bytes:
    out = bytearray()
    for b in body:
        if b in (GDL90_FLAG, GDL90_ESCAPE):
            out.append(GDL90_ESCAPE)
            out.append(b ^ GDL90_ESCAPE_XOR)
        else:
            out.append(b)
    return bytes(out)


def gdl90_frame(msg_id: int, payload: bytes) -> bytes:
    body = bytes([msg_id & 0x7F]) + payload
    crc = gdl90_crc16(body)
    body += struct.pack("<H", crc)  # little-endian CRC per ICD
    return bytes([GDL90_FLAG]) + gdl90_escape(body) + bytes([GDL90_FLAG])


def _clamp(x: float, lo: float, hi: float) -> float:
    return lo if x < lo else hi if x > hi else x


def degrees_to_semicircles24(deg: float) -> int:
    """
    Convert degrees to signed 24-bit semicircle fraction:
      deg = raw * 180 / 2^23
      raw = deg * 2^23 / 180
    """
    raw = int(round(deg * (1 << 23) / 180.0))
    raw = max(-(1 << 23), min((1 << 23) - 1, raw))
    return raw


def pack_be24_signed(value: int) -> bytes:
    value &= 0xFFFFFF
    return bytes([(value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF])


def altitude_ft_to_code(altitude_ft: float) -> int:
    """
    GDL90 altitude code: 25ft steps, offset -1000ft, 12-bit.
    """
    code = int(round((altitude_ft + 1000.0) / 25.0))
    return max(0, min(0xFFE, code))


def callsign8(s: str) -> bytes:
    s = "".join(ch for ch in s.upper() if 0x20 <= ord(ch) < 0x7F)
    s = s[:8].ljust(8, " ")
    return s.encode("ascii", errors="replace")


def build_heartbeat(*, include_tod: bool = True) -> bytes:
    """
    Heartbeat payload (6 bytes). If include_tod is false, all zeros (no UTC
    time-of-day, status2 bit0 clear). If true, encode UTC seconds since
    midnight with UTC-valid bit (Stratux-compatible layout).
    """
    payload = bytearray(6)
    if include_tod:
        now = datetime.now(timezone.utc)
        seconds = now.hour * 3600 + now.minute * 60 + now.second
        # Match ICD §3.1 / Stratux: [0] status1, [1] status2 (UTC OK bit0,
        # seconds bit16 in bit7), [2:3] seconds LE, [4:5] message counts.
        payload[0] = 0x01
        payload[1] = 0x01 | (((seconds >> 16) & 0x01) << 7)
        payload[2] = seconds & 0xFF
        payload[3] = (seconds >> 8) & 0xFF
    return bytes(payload)


def build_foreflight_id(*, geo_altitude_is_msl: bool) -> bytes:
    """
    ForeFlight GDL90 extension ID message (0x65, sub-id 0, version 1).

    Capabilities mask bit 0 declares the datum for Ownship Geometric Altitude
    (0x0B): 0=WGS-84 ellipsoid, 1=MSL.
    """
    payload = bytearray(38)
    payload[0] = 0x00  # sub-id (ID)
    payload[1] = 0x01  # version

    # payload[34..37] capabilities mask (big-endian)
    capabilities = 0x00000001 if geo_altitude_is_msl else 0x00000000
    payload[34:38] = struct.pack(">I", capabilities)
    return bytes(payload)


def build_foreflight_ahrs(*, roll_deg: Optional[float], pitch_deg: Optional[float],
                          heading_deg: Optional[float], heading_is_magnetic: bool,
                          ias_kt: Optional[float], tas_kt: Optional[float]) -> bytes:
    """
    ForeFlight GDL90 extension AHRS message (0x65, sub-id 0x01).

    Roll/Pitch/Heading use 0.1° resolution. IAS/TAS use knots. 0x7fff/0xffff
    represent invalid values depending on the field.
    """
    payload = bytearray(11)
    payload[0] = 0x01  # sub-id (AHRS)

    def pack_i16_tenths(x: Optional[float]) -> bytes:
        if x is None:
            return struct.pack(">h", 0x7FFF)
        v = int(round(_clamp(x, -180.0, 180.0) * 10.0))
        return struct.pack(">h", v)

    payload[1:3] = pack_i16_tenths(roll_deg)
    payload[3:5] = pack_i16_tenths(pitch_deg)

    if heading_deg is None:
        payload[5:7] = struct.pack(">H", 0xFFFF)
    else:
        h10 = int(round((_clamp(heading_deg, 0.0, 359.9)) * 10.0)) & 0x7FFF
        h = h10 | (0x8000 if heading_is_magnetic else 0x0000)
        payload[5:7] = struct.pack(">H", h)

    def pack_u16(x: Optional[float]) -> bytes:
        if x is None:
            return struct.pack(">H", 0xFFFF)
        v = int(round(_clamp(x, 0.0, 65534.0)))
        return struct.pack(">H", v)

    payload[7:9] = pack_u16(ias_kt)
    payload[9:11] = pack_u16(tas_kt)
    return bytes(payload)


def build_ownship(lat_deg: float, lon_deg: float, altitude_ft: float,
                  ground_speed_kt: float, track_deg: float) -> bytes:
    payload = bytearray(27)
    # bytes 0..3 reserved/status: leave 0
    payload[4:7] = pack_be24_signed(degrees_to_semicircles24(lat_deg))
    payload[7:10] = pack_be24_signed(degrees_to_semicircles24(lon_deg))

    misc = 0x01  # set track_valid bits (driver checks (misc&0x03)!=0)
    alt_code = altitude_ft_to_code(altitude_ft)
    alt_word = (alt_code << 4) | (misc & 0x0F)
    payload[10:12] = struct.pack(">H", alt_word)

    payload[12] = 0x00  # NACp/NIC (unused)

    h = int(round(_clamp(ground_speed_kt, 0.0, 4094.0)))
    v = 0x0800  # vertical N/A
    vel = ((h & 0x0FFF) << 12) | (v & 0x0FFF)
    payload[13:16] = bytes([(vel >> 16) & 0xFF, (vel >> 8) & 0xFF, vel & 0xFF])

    track_byte = int(round((_clamp(track_deg, 0.0, 359.0) / 360.0) * 256.0)) & 0xFF
    payload[16] = track_byte

    payload[17] = 0x01  # emitter category (unused)
    payload[18:26] = callsign8("OWN")
    payload[26] = 0x00
    return bytes(payload)


def build_ownship_geo_altitude(*, altitude_ft: float) -> bytes:
    """
    Ownship Geometric Altitude message payload (0x0B).
    Signed int16 with 5ft resolution; followed by 2 bytes of vertical metrics.
    """
    alt5 = int(round(altitude_ft / 5.0))
    alt5 = max(-32768, min(32767, alt5))
    return struct.pack(">hH", alt5, 0x0000)

def build_ownship_test_vector() -> bytes:
    """
    Ownship report payload taken from XCSoar's unit tests (unescaped frame
    between msg_id and CRC). Useful to verify receiver setup.
    """
    return bytes([
        0x00, 0x00, 0x00, 0x00,
        0x15, 0xA7, 0xE5,
        0xBA, 0x47, 0x99,
        0x08, 0xC9,
        0x88,
        0xFF, 0xE0, 0x00,
        0x80,
        0x01,
        0x4E, 0x31, 0x32, 0x33, 0x34, 0x35, 0x20, 0x20,
        0x00,
    ])


def build_traffic(icao: int, lat_deg: float, lon_deg: float,
                  altitude_ft: Optional[float],
                  ground_speed_kt: Optional[float],
                  track_deg: Optional[float],
                  callsign: str,
                  address_type: int = 0) -> bytes:
    payload = bytearray(27)
    alert_status = 0
    payload[0] = ((address_type & 0x0F) << 4) | (alert_status & 0x0F)
    payload[1:4] = bytes([(icao >> 16) & 0xFF, (icao >> 8) & 0xFF, icao & 0xFF])
    payload[4:7] = pack_be24_signed(degrees_to_semicircles24(lat_deg))
    payload[7:10] = pack_be24_signed(degrees_to_semicircles24(lon_deg))

    misc = 0x01  # track_valid bits
    if altitude_ft is None:
        alt_code = 0xFFF
    else:
        alt_code = altitude_ft_to_code(altitude_ft)
    alt_word = (alt_code << 4) | (misc & 0x0F)
    payload[10:12] = struct.pack(">H", alt_word)

    payload[12] = 0x00  # NACp/NIC (unused)

    if ground_speed_kt is None:
        h = 0x0FFF
    else:
        h = int(round(_clamp(ground_speed_kt, 0.0, 4094.0)))
    v = 0x0800  # vertical N/A
    vel = ((h & 0x0FFF) << 12) | (v & 0x0FFF)
    payload[13:16] = bytes([(vel >> 16) & 0xFF, (vel >> 8) & 0xFF, vel & 0xFF])

    if track_deg is None:
        payload[16] = 0xFF
    else:
        track_byte = int(round((_clamp(track_deg, 0.0, 359.0) / 360.0) * 256.0)) & 0xFF
        payload[16] = track_byte

    payload[17] = 0x01  # emitter category
    payload[18:26] = callsign8(callsign)
    payload[26] = 0x00
    return bytes(payload)


@dataclass
class Target:
    icao: int
    callsign: str
    radius_m: float
    altitude_ft: float
    speed_kt: float
    alt_phase: float
    north_m: float
    east_m: float
    vn_mps: float
    ve_mps: float


def step_targets(rng: random.Random, targets: list[Target],
                 dt: float) -> None:
    # Simple bounded random walk with damping (stable + smooth).
    # Acceleration noise (m/s^2):
    a_sigma = 2.0
    # Damping (1/s):
    damping = 0.15

    for t in targets:
        an = rng.gauss(0.0, a_sigma)
        ae = rng.gauss(0.0, a_sigma)

        t.vn_mps += (an - damping * t.vn_mps) * dt
        t.ve_mps += (ae - damping * t.ve_mps) * dt

        t.north_m += t.vn_mps * dt
        t.east_m += t.ve_mps * dt

        r = math.hypot(t.north_m, t.east_m)
        if r > t.radius_m and r > 1e-3:
            # Clamp and reflect velocity at boundary.
            scale = t.radius_m / r
            t.north_m *= scale
            t.east_m *= scale
            t.vn_mps = -0.5 * t.vn_mps
            t.ve_mps = -0.5 * t.ve_mps


def _offset_lat_lon_m(lat0_deg: float, lon0_deg: float,
                      north_m: float, east_m: float) -> tuple[float, float]:
    # Small-angle approximation: good enough for synthetic local traffic.
    lat0 = math.radians(lat0_deg)
    dlat = north_m / 111_320.0
    dlon = east_m / (111_320.0 * max(0.2, math.cos(lat0)))
    return lat0_deg + dlat, lon0_deg + dlon


def iter_messages(now: float, own_lat: float, own_lon: float,
                  own_alt_ft: float, own_track_deg: float,
                  targets: Iterable[Target],
                  send_ownship: bool,
                  ownship_test_vector: bool,
                  send_foreflight_id: bool,
                  foreflight_geo_altitude_is_msl: bool,
                  send_foreflight_ahrs: bool,
                  foreflight_ahrs_heading_is_magnetic: bool,
                  foreflight_ahrs_ias_kt: Optional[float],
                  foreflight_ahrs_tas_kt: Optional[float],
                  foreflight_ahrs_roll_amp_deg: float,
                  foreflight_ahrs_roll_period_s: float,
                  foreflight_ahrs_pitch_amp_deg: float,
                  foreflight_ahrs_pitch_period_s: float,
                  send_geo_altitude: bool,
                  target_alt_amp_ft: float,
                  target_alt_period_s: float,
                  traffic_before_ownship: bool,
                  heartbeat_include_tod: bool) -> list[bytes]:
    out: list[bytes] = []

    if send_foreflight_id:
        out.append(gdl90_frame(
            0x65,
            build_foreflight_id(geo_altitude_is_msl=foreflight_geo_altitude_is_msl),
        ))

    if send_foreflight_ahrs:
        roll = None
        if foreflight_ahrs_roll_amp_deg > 0 and foreflight_ahrs_roll_period_s > 0:
            roll = foreflight_ahrs_roll_amp_deg * math.sin(
                now * 2.0 * math.pi / foreflight_ahrs_roll_period_s
            )

        pitch = None
        if foreflight_ahrs_pitch_amp_deg > 0 and foreflight_ahrs_pitch_period_s > 0:
            pitch = foreflight_ahrs_pitch_amp_deg * math.sin(
                now * 2.0 * math.pi / foreflight_ahrs_pitch_period_s
            )

        out.append(gdl90_frame(
            0x65,
            build_foreflight_ahrs(
                roll_deg=roll,
                pitch_deg=pitch,
                heading_deg=own_track_deg,
                heading_is_magnetic=foreflight_ahrs_heading_is_magnetic,
                ias_kt=foreflight_ahrs_ias_kt,
                tas_kt=foreflight_ahrs_tas_kt,
            ),
        ))

    out.append(gdl90_frame(
        0x00, build_heartbeat(include_tod=heartbeat_include_tod)))

    def append_traffic() -> None:
        for t in targets:
            lat, lon = _offset_lat_lon_m(own_lat, own_lon, t.north_m, t.east_m)

            track = None
            if abs(t.vn_mps) > 0.5 or abs(t.ve_mps) > 0.5:
                track = (math.degrees(math.atan2(t.ve_mps, t.vn_mps)) + 360.0) % 360.0

            if target_alt_amp_ft > 0 and target_alt_period_s > 0:
                alt_wave = math.sin((now * 2.0 * math.pi / target_alt_period_s) + t.alt_phase)
                alt_ft = t.altitude_ft + target_alt_amp_ft * alt_wave
            else:
                alt_ft = t.altitude_ft

            out.append(gdl90_frame(
                0x14,
                build_traffic(
                    t.icao,
                    lat,
                    lon,
                    altitude_ft=alt_ft,
                    ground_speed_kt=t.speed_kt,
                    track_deg=track,
                    callsign=t.callsign,
                    address_type=0,
                ),
            ))

    def append_ownship() -> None:
        if not send_ownship:
            return

        payload = build_ownship_test_vector() if ownship_test_vector else \
            build_ownship(own_lat, own_lon, own_alt_ft, 40.0, own_track_deg)
        out.append(gdl90_frame(0x0A, payload))

        if send_geo_altitude:
            out.append(gdl90_frame(
                0x0B,
                build_ownship_geo_altitude(altitude_ft=own_alt_ft),
            ))

    # Optionally send traffic frames before ownship to simulate "no position"
    # targets briefly (relative vectors not computed until ownship is known).
    if traffic_before_ownship:
        append_traffic()
        append_ownship()
    else:
        append_ownship()
        append_traffic()

    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Send synthetic GDL90 via UDP")
    ap.add_argument("--host", default="127.0.0.1", help="XCSoar host/IP")
    ap.add_argument("--port", type=int, default=4352, help="XCSoar UDP port")
    ap.add_argument("--rate", type=float, default=1.0, help="Packets per second")
    ap.add_argument("--n-targets", type=int, default=6, help="Number of traffic targets")
    ap.add_argument("--seed", type=int, default=1, help="RNG seed (stable target IDs)")
    ap.add_argument("--own-lat", type=float, default=50.0, help="Ownship latitude (deg)")
    ap.add_argument("--own-lon", type=float, default=8.0, help="Ownship longitude (deg)")
    ap.add_argument("--own-alt-ft", type=float, default=1500.0, help="Ownship altitude (ft)")
    ap.add_argument("--target-alt-amp-ft", type=float, default=200.0,
                    help="Target altitude oscillation amplitude (ft, 0=disabled)")
    ap.add_argument("--target-alt-period", type=float, default=60.0,
                    help="Target altitude oscillation period (s)")
    ap.add_argument("--traffic-before-ownship", action="store_true",
                    help="Send traffic (0x14) before ownship (0x0A) each cycle")
    ap.add_argument("--no-heartbeat-tod", action="store_true",
                    help="Heartbeat (0x00): omit UTC time-of-day (six zero "
                         "payload bytes; XCSoar will not set GPS time from it)")
    ap.add_argument("--foreflight-id", action="store_true",
                    help="Send ForeFlight ID (0x65 sub-id 0, version 1) each cycle")
    ap.add_argument("--foreflight-geo-altitude-msl", action="store_true",
                    help="In ForeFlight ID, set capability bit0: 0x0B datum is MSL (default: ellipsoid)")
    ap.add_argument("--foreflight-ahrs", action="store_true",
                    help="Send ForeFlight AHRS (0x65 sub-id 1) each cycle")
    ap.add_argument("--foreflight-ahrs-heading-magnetic", action="store_true",
                    help="Mark ForeFlight AHRS heading as magnetic (default: true heading)")
    ap.add_argument("--foreflight-ahrs-ias-kt", type=float, default=60.0,
                    help="ForeFlight AHRS IAS value (knots)")
    ap.add_argument("--foreflight-ahrs-tas-kt", type=float, default=65.0,
                    help="ForeFlight AHRS TAS value (knots)")
    ap.add_argument("--foreflight-ahrs-roll-amp-deg", type=float, default=20.0,
                    help="ForeFlight AHRS roll oscillation amplitude (deg, 0=invalid)")
    ap.add_argument("--foreflight-ahrs-roll-period", type=float, default=12.0,
                    help="ForeFlight AHRS roll oscillation period (s)")
    ap.add_argument("--foreflight-ahrs-pitch-amp-deg", type=float, default=8.0,
                    help="ForeFlight AHRS pitch oscillation amplitude (deg, 0=invalid)")
    ap.add_argument("--foreflight-ahrs-pitch-period", type=float, default=9.0,
                    help="ForeFlight AHRS pitch oscillation period (s)")
    ap.add_argument("--geo-altitude", action="store_true",
                    help="Send Ownship Geometric Altitude (0x0B) each cycle")
    ap.add_argument("--no-ownship", action="store_true",
                    help="Do not send Ownship (0x0A) reports")
    ap.add_argument("--ownship-test-vector", action="store_true",
                    help="Send the unit-test Ownship vector (for receiver setup)")
    ap.add_argument("--packet-mode", choices=["concat", "single"], default="single",
                    help="UDP packetization: 'single' sends one GDL90 frame per packet; "
                         "'concat' concatenates all frames into one packet")
    ap.add_argument("--stdout-hex", action="store_true",
                    help="Print frames as hex instead of sending")
    args = ap.parse_args()

    rng = random.Random(args.seed)

    targets: list[Target] = []
    for i in range(max(0, args.n_targets)):
        icao = (0xA00000 + i + rng.randrange(0, 0x0FFF)) & 0xFFFFFF
        callsign = f"T{i+1:02d}"
        radius_m = 800.0 + 200.0 * i
        ang = rng.random() * 2.0 * math.pi
        north = radius_m * math.cos(ang)
        east = radius_m * math.sin(ang)
        targets.append(Target(
            icao=icao,
            callsign=callsign,
            radius_m=radius_m,
            altitude_ft=args.own_alt_ft + 300.0 + 100.0 * i,
            speed_kt=55.0,
            alt_phase=rng.random() * 2.0 * math.pi,
            north_m=north,
            east_m=east,
            vn_mps=rng.uniform(-5.0, 5.0),
            ve_mps=rng.uniform(-5.0, 5.0),
        ))

    if args.stdout_hex:
        now = time.monotonic()
        msgs = iter_messages(now, args.own_lat, args.own_lon, args.own_alt_ft,
                             own_track_deg=0.0, targets=targets,
                             send_ownship=not args.no_ownship,
                             ownship_test_vector=args.ownship_test_vector,
                             send_foreflight_id=args.foreflight_id,
                             foreflight_geo_altitude_is_msl=args.foreflight_geo_altitude_msl,
                             send_foreflight_ahrs=args.foreflight_ahrs,
                             foreflight_ahrs_heading_is_magnetic=args.foreflight_ahrs_heading_magnetic,
                             foreflight_ahrs_ias_kt=args.foreflight_ahrs_ias_kt,
                             foreflight_ahrs_tas_kt=args.foreflight_ahrs_tas_kt,
                             foreflight_ahrs_roll_amp_deg=args.foreflight_ahrs_roll_amp_deg,
                             foreflight_ahrs_roll_period_s=args.foreflight_ahrs_roll_period,
                             foreflight_ahrs_pitch_amp_deg=args.foreflight_ahrs_pitch_amp_deg,
                             foreflight_ahrs_pitch_period_s=args.foreflight_ahrs_pitch_period,
                             send_geo_altitude=args.geo_altitude,
                             target_alt_amp_ft=args.target_alt_amp_ft,
                             target_alt_period_s=args.target_alt_period,
                             traffic_before_ownship=args.traffic_before_ownship,
                             heartbeat_include_tod=not args.no_heartbeat_tod)
        for m in msgs:
            print(m.hex())
        return 0

    addr = (args.host, args.port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    period = 1.0 / max(0.1, args.rate)
    next_t = time.monotonic()
    own_track = 0.0
    last_now = None

    try:
        while True:
            now = time.monotonic()
            if now < next_t:
                time.sleep(next_t - now)
                continue

            if last_now is None:
                last_now = now
            dt = now - last_now
            last_now = now
            step_targets(rng, targets, dt)

            own_track = (own_track - 2.0) % 360.0
            msgs = iter_messages(now, args.own_lat, args.own_lon, args.own_alt_ft,
                                 own_track_deg=own_track, targets=targets,
                                 send_ownship=not args.no_ownship,
                                 ownship_test_vector=args.ownship_test_vector,
                                 send_foreflight_id=args.foreflight_id,
                                 foreflight_geo_altitude_is_msl=args.foreflight_geo_altitude_msl,
                                 send_foreflight_ahrs=args.foreflight_ahrs,
                                 foreflight_ahrs_heading_is_magnetic=args.foreflight_ahrs_heading_magnetic,
                                 foreflight_ahrs_ias_kt=args.foreflight_ahrs_ias_kt,
                                 foreflight_ahrs_tas_kt=args.foreflight_ahrs_tas_kt,
                                 foreflight_ahrs_roll_amp_deg=args.foreflight_ahrs_roll_amp_deg,
                                 foreflight_ahrs_roll_period_s=args.foreflight_ahrs_roll_period,
                                 foreflight_ahrs_pitch_amp_deg=args.foreflight_ahrs_pitch_amp_deg,
                                 foreflight_ahrs_pitch_period_s=args.foreflight_ahrs_pitch_period,
                                 send_geo_altitude=args.geo_altitude,
                                 target_alt_amp_ft=args.target_alt_amp_ft,
                                 target_alt_period_s=args.target_alt_period,
                                 traffic_before_ownship=args.traffic_before_ownship,
                                 heartbeat_include_tod=not args.no_heartbeat_tod)

            if args.packet_mode == "concat":
                sock.sendto(b"".join(msgs), addr)
            else:
                for m in msgs:
                    sock.sendto(m, addr)

            next_t += period
    except KeyboardInterrupt:
        return 0
    finally:
        sock.close()


if __name__ == "__main__":
    raise SystemExit(main())

