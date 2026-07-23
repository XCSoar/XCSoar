#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
#
# Feed Condor 3 Spectate.json multiplayer traffic to XCSoar as FLARM NMEA.
#
# Reads Spectate.json once per second and connects to XCSoar's TCP port
# listener (default 127.0.0.1:2000). Sends $PFLAA traffic plus $PFLAM
# messaging (pilot, callsign, registration, aircraft type).
# Configure the XCSoar device as:
#   Port type: TCP port
#   Port: 2000
#   Driver: FLARM (or any NMEA driver — $PFLAA is parsed generically)
# Condor3UDP on another port carries own-ship data.
#
# Examples:
#   ./tools/condor-spectate-flarm.py --smb 10.21.30.204 --own-cn OL --log /tmp/condor-flarm.log
#   ./tools/condor-spectate-flarm.py --file /mnt/c3sim-logs/Spectate.json --ref-id 1110252881

from __future__ import annotations

import argparse
import json
import math
import socket
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def nmea_checksum(body: str) -> str:
    cs = 0
    for ch in body:
        cs ^= ord(ch)
    return f"{cs:02X}"


def nmea_line(sentence: str) -> str:
    assert sentence.startswith("$")
    return f"{sentence}*{nmea_checksum(sentence[1:])}"


def parse_condor_coord(text: str) -> float:
    """Condor spectate coords like N45.9008 or W007.1234 (decimal degrees)."""
    text = text.strip()
    if not text:
        raise ValueError("empty coordinate")
    hemi = text[0].upper()
    value = float(text[1:])
    if hemi in ("S", "W"):
        value = -value
    elif hemi not in ("N", "E"):
        raise ValueError(f"bad hemisphere in {text!r}")
    return value


def relative_bearing(rel_n: float, rel_e: float) -> float:
    return math.degrees(math.atan2(rel_e, rel_n)) % 360.0


def bearing_delta(a: float, b: float) -> float:
    d = (b - a) % 360.0
    return d if d <= 180.0 else d - 360.0


@dataclass
class TrafficSnapshot:
    cn: str
    player_id: str
    raw_lat: str
    raw_lon: str
    raw_alt: float
    rel_n_raw: float
    rel_e_raw: float
    rel_v_raw: float
    rel_n_out: float
    rel_e_out: float
    rel_v_out: float
    bearing_raw: float
    bearing_out: float
    distance_m: float


@dataclass
class BuildSnapshot:
    ref_source: str
    ref_lat: float
    ref_lon: float
    ref_alt: float
    ref_raw_lat: str
    ref_raw_lon: str
    own_cn: str | None
    player_count: int
    traffic_count: int
    udp_ref: tuple[float, float, float] | None
    traffic: list[TrafficSnapshot] = field(default_factory=list)


class DebugLogger:
    """Append-only debug log for diagnosing relative-position flips."""

    def __init__(self, path: Path, flip_warn_deg: float = 45.0) -> None:
        self._path = path
        self._flip_warn_deg = flip_warn_deg
        self._lock = threading.Lock()
        self._last_bearing: dict[str, float] = {}
        self._path.parent.mkdir(parents=True, exist_ok=True)
        self.write(f"# condor-spectate-flarm log started {self._stamp()}")

    @staticmethod
    def _stamp() -> str:
        return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    def write(self, msg: str) -> None:
        with self._lock:
            with self._path.open("a", encoding="utf-8") as f:
                f.write(msg + "\n")

    def log_build(self, snap: BuildSnapshot) -> None:
        ts = self._stamp()
        lines = [
            f"{ts} ref={snap.ref_source} own={snap.own_cn or '-'} "
            f"players={snap.player_count} traffic={snap.traffic_count}",
            f"  ref_raw={snap.ref_raw_lat}/{snap.ref_raw_lon} "
            f"ref={snap.ref_lat:.6f},{snap.ref_lon:.6f} alt={snap.ref_alt:.0f}",
        ]
        if snap.udp_ref is not None:
            ulat, ulon, ualt = snap.udp_ref
            d_n, d_e = relative_ne_m(snap.ref_lat, snap.ref_lon, ulat, ulon)
            lines.append(
                f"  udp_ref={ulat:.6f},{ulon:.6f} alt={ualt:.0f} "
                f"delta_from_ref N={d_n:.0f} E={d_e:.0f}"
            )
        for t in snap.traffic:
            dist_raw = math.hypot(t.rel_n_raw, t.rel_e_raw)
            dist_out = math.hypot(t.rel_n_out, t.rel_e_out)
            lines.append(
                f"  {t.cn} id={t.player_id} raw={t.raw_lat}/{t.raw_lon} "
                f"alt={t.raw_alt:.0f}"
            )
            lines.append(
                f"    rel_raw N={t.rel_n_raw:.0f} E={t.rel_e_raw:.0f} "
                f"V={t.rel_v_raw:.0f} brg={t.bearing_raw:.1f} dist={dist_raw:.0f}"
            )
            lines.append(
                f"    rel_out N={t.rel_n_out:.0f} E={t.rel_e_out:.0f} "
                f"V={t.rel_v_out:.0f} brg={t.bearing_out:.1f} dist={dist_out:.0f}"
            )
            prev = self._last_bearing.get(t.player_id)
            if prev is not None:
                d_raw = bearing_delta(prev, t.bearing_raw)
                d_out = bearing_delta(prev, t.bearing_out)
                if (abs(d_raw) >= self._flip_warn_deg or
                        abs(d_out) >= self._flip_warn_deg):
                    lines.append(
                        f"    FLIP brg_change raw={d_raw:+.1f} "
                        f"smooth={d_out:+.1f} (prev={prev:.1f})"
                    )
            self._last_bearing[t.player_id] = t.bearing_out
        self.write("\n".join(lines))


def relative_ne_m(own_lat: float, own_lon: float, lat: float, lon: float) -> tuple[float, float]:
    lat_rad = math.radians(own_lat)
    d_lat = math.radians(lat - own_lat)
    d_lon = math.radians(lon - own_lon)
    north = d_lat * 6378137.0
    east = d_lon * 6378137.0 * math.cos(lat_rad)
    return north, east


class OwnShipPosition:
    """Latest own-ship position from Condor UDP (optional)."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self.lat: float | None = None
        self.lon: float | None = None
        self.alt: float | None = None

    def update(self, lat: float, lon: float, alt: float | None) -> None:
        with self._lock:
            self.lat = lat
            self.lon = lon
            if alt is not None:
                self.alt = alt

    def snapshot(self) -> tuple[float, float, float] | None:
        with self._lock:
            if self.lat is None or self.lon is None:
                return None
            return self.lat, self.lon, self.alt or 0.0


def parse_condor_udp(data: bytes) -> dict[str, float]:
    values: dict[str, float] = {}
    for line in data.decode("ascii", errors="ignore").splitlines():
        if "=" not in line:
            continue
        key, _, raw = line.partition("=")
        try:
            values[key.strip().lower()] = float(raw.strip())
        except ValueError:
            pass
    return values


def start_udp_listener(port: int, own: OwnShipPosition) -> threading.Thread:
    """Background thread: read Condor UDP for high-precision own-ship."""

    def run() -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        if hasattr(socket, "SO_REUSEPORT"):
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.bind(("", port))
        while True:
            data, _addr = sock.recvfrom(8192)
            parsed = parse_condor_udp(data)
            lat = parsed.get("latitude")
            lon = parsed.get("longitude")
            if lat is not None and lon is not None:
                own.update(lat, lon, parsed.get("altitude"))

    thread = threading.Thread(target=run, name="condor-udp", daemon=True)
    thread.start()
    return thread


class RelativeSmoother:
    """EMA filter on relative N/E/V to tame Spectate.json quantisation."""

    def __init__(self, tau_s: float) -> None:
        self.tau_s = tau_s
        self._state: dict[str, tuple[float, float, float, float]] = {}

    def filter(self, player_id: str, rel_n: float, rel_e: float,
               rel_v: float, now: float) -> tuple[float, float, float]:
        if self.tau_s <= 0:
            return rel_n, rel_e, rel_v

        prev = self._state.get(player_id)
        if prev is None:
            self._state[player_id] = (rel_n, rel_e, rel_v, now)
            return rel_n, rel_e, rel_v

        pn, pe, pv, pt = prev
        dt = max(now - pt, 0.001)
        alpha = min(1.0, dt / self.tau_s)
        sn = pn + alpha * (rel_n - pn)
        se = pe + alpha * (rel_e - pe)
        sv = pv + alpha * (rel_v - pv)
        self._state[player_id] = (sn, se, sv, now)
        return sn, se, sv


def flarm_hex_id(player: dict[str, Any]) -> str:
    """Six-digit FLARM id (PFLAM and PFLAA hex part)."""
    raw = str(player.get("ID", "") or "").strip()
    try:
        n = int(raw, 10)
    except ValueError:
        n = abs(hash(raw))
    n = (n & 0xFFFFFF) or 1
    return f"{n:06X}"


def flarm_pflaa_id(player: dict[str, Any]) -> str:
    """PFLAA id field: hex id plus optional !callsign suffix."""
    hex_id = flarm_hex_id(player)
    cn = str(player.get("CN", "") or "").strip()
    if cn:
        return f"{hex_id}!{cn}"
    rn = str(player.get("RN", "") or "").strip()
    if rn:
        return f"{hex_id}!{rn}"
    return hex_id


def truncate_utf8(text: str, max_bytes: int) -> str:
    raw = text.encode("utf-8")
    if len(raw) <= max_bytes:
        return text
    while max_bytes > 0:
        try:
            return raw[:max_bytes].decode("utf-8")
        except UnicodeDecodeError:
            max_bytes -= 1
    return ""


def hex_payload(text: str, max_bytes: int = 17) -> str:
    """Hex-encode a FLARM PFLAM text payload (max 17 UTF-8 bytes)."""
    return truncate_utf8(text.strip(), max_bytes).encode("utf-8").hex().upper()


def pflam(hex_id: str, msg_type: str, payload: str) -> str:
    # PFLAM,U,<IDType>,<ID>,<MsgType>,<Payload> — IDType 2 per FTD-109.
    return nmea_line(f"$PFLAM,U,2,{hex_id},{msg_type},{payload}")


def pflam_for_player(player: dict[str, Any]) -> list[str]:
    hex_id = flarm_hex_id(player)
    lines: list[str] = []

    first = str(player.get("firstname", "") or "").strip()
    last = str(player.get("lastname", "") or "").strip()
    pilot = f"{first} {last}".strip()
    if pilot:
        lines.append(pflam(hex_id, "PNAME", hex_payload(pilot)))

    cn = str(player.get("CN", "") or "").strip()
    if cn:
        lines.append(pflam(hex_id, "ACALL", hex_payload(cn)))

    rn = str(player.get("RN", "") or "").strip()
    if rn:
        lines.append(pflam(hex_id, "AREG", hex_payload(rn)))

    plane = str(player.get("plane", "") or "").strip()
    if plane:
        lines.append(pflam(hex_id, "ATYPE", hex_payload(plane)))

    return lines


def pflaa(player: dict[str, Any], rel_n: float, rel_e: float, rel_v: float) -> str:
    track = int(float(player.get("heading", 0) or 0)) % 360
    speed_raw = float(player.get("speed", 0) or 0)
    # Condor spectate speed is already km/h; FLARM PFLAA expects km/h.
    speed_kmh = max(0.0, speed_raw)
    climb = float(player.get("vario", 0) or 0)
    body = (
        f"$PFLAA,0,{int(round(rel_n))},{int(round(rel_e))},"
        f"{int(round(rel_v))},1,{flarm_pflaa_id(player)},{track},0,"
        f"{speed_kmh:.0f},{climb:.1f},1"
    )
    return nmea_line(body)


def pflau(traffic_count: int) -> str:
    # RX=1, TX=1, GPS=2 (3D fix), Power=1, no alarm.
    return nmea_line(f"$PFLAU,1,1,2,1,0,0,0,0,{traffic_count}")


def load_spectate_bytes(raw: bytes) -> list[dict[str, Any]]:
    data = json.loads(raw.decode("utf-8-sig"))
    if isinstance(data, list):
        return [x for x in data if isinstance(x, dict)]
    if isinstance(data, dict):
        return [data]
    raise ValueError("Spectate.json must be a JSON array or object")


def load_spectate_file(path: Path) -> list[dict[str, Any]]:
    with open(path, "rb") as f:
        return load_spectate_bytes(f.read())


def read_smb(host: str, share: str, user: str, password: str,
             remote_name: str) -> bytes:
    """Read a share file read-only via a short-lived SMB session.

    Uses ``get remote -`` (stdout only, no local copy). Each poll opens,
    reads, and closes immediately so Condor can keep writing Spectate.json.
    """
    url = f"//{host}/{share}"
    cmd = [
        "smbclient", url, "-U", f"{user}%{password}",
        "-c", f"get {remote_name} -",
    ]
    result = subprocess.run(cmd, check=True, stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, timeout=30.0)
    return result.stdout


def find_own_ship(players: list[dict[str, Any]], ref_id: str | None,
                  own_cn: str | None) -> dict[str, Any] | None:
    """Return the own-ship record, or None if not specified/found."""
    if ref_id:
        for p in players:
            if str(p.get("ID", "")) == ref_id:
                return p
        raise ValueError(f"own-ship id {ref_id!r} not in Spectate.json")
    if own_cn:
        for p in players:
            if str(p.get("CN", "") or "").strip() == own_cn:
                return p
        raise ValueError(f"own-ship callsign {own_cn!r} not in Spectate.json")
    return None


def is_own_ship(player: dict[str, Any], own: dict[str, Any] | None) -> bool:
    if own is None:
        return False
    return str(player.get("ID", "")) == str(own.get("ID", ""))


def build_sentences(players: list[dict[str, Any]], ref_id: str | None,
                    own_cn: str | None,
                    own_pos: OwnShipPosition | None = None,
                    smoother: RelativeSmoother | None = None,
                    now: float | None = None,
                    debug: DebugLogger | None = None) -> list[str]:
    if not players:
        return [pflau(0)]

    tick = time.monotonic() if now is None else now
    own = find_own_ship(players, ref_id, own_cn)
    traffic = [p for p in players if not is_own_ship(p, own)]

    udp_ref = own_pos.snapshot() if own_pos is not None else None
    ref_source = "spectate-first"
    ref_raw_lat = ref_raw_lon = ""
    # Prefer own-ship from the same Spectate snapshot so traffic relative
    # coords are self-consistent.  UDP own position is async and can differ.
    if own is not None:
        ref_source = "spectate-own"
        ref_raw_lat = str(own["latitude"])
        ref_raw_lon = str(own["longitude"])
        ref_lat = parse_condor_coord(ref_raw_lat)
        ref_lon = parse_condor_coord(ref_raw_lon)
        ref_alt = float(own.get("altitude", 0) or 0)
    elif udp_ref is not None:
        ref_source = "udp"
        ref_lat, ref_lon, ref_alt = udp_ref
    else:
        ref = players[0]
        ref_raw_lat = str(ref["latitude"])
        ref_raw_lon = str(ref["longitude"])
        ref_lat = parse_condor_coord(ref_raw_lat)
        ref_lon = parse_condor_coord(ref_raw_lon)
        ref_alt = float(ref.get("altitude", 0) or 0)

    smooth = smoother or RelativeSmoother(0)
    own_label = str(own.get("CN", "")).strip() if own else own_cn
    snap = BuildSnapshot(
        ref_source=ref_source,
        ref_lat=ref_lat,
        ref_lon=ref_lon,
        ref_alt=ref_alt,
        ref_raw_lat=ref_raw_lat,
        ref_raw_lon=ref_raw_lon,
        own_cn=own_label or None,
        player_count=len(players),
        traffic_count=len(traffic),
        udp_ref=udp_ref,
    )

    lines: list[str] = []
    for p in traffic:
        lines.extend(pflam_for_player(p))
        raw_lat = str(p["latitude"])
        raw_lon = str(p["longitude"])
        lat = parse_condor_coord(raw_lat)
        lon = parse_condor_coord(raw_lon)
        alt = float(p.get("altitude", 0) or 0)
        rel_n, rel_e = relative_ne_m(ref_lat, ref_lon, lat, lon)
        rel_v = alt - ref_alt
        rel_n_raw, rel_e_raw, rel_v_raw = rel_n, rel_e, rel_v
        player_id = str(p.get("ID", ""))
        rel_n, rel_e, rel_v = smooth.filter(player_id, rel_n, rel_e, rel_v, tick)
        cn = str(p.get("CN", "") or "").strip() or player_id
        snap.traffic.append(TrafficSnapshot(
            cn=cn,
            player_id=player_id,
            raw_lat=raw_lat,
            raw_lon=raw_lon,
            raw_alt=alt,
            rel_n_raw=rel_n_raw,
            rel_e_raw=rel_e_raw,
            rel_v_raw=rel_v_raw,
            rel_n_out=rel_n,
            rel_e_out=rel_e,
            rel_v_out=rel_v,
            bearing_raw=relative_bearing(rel_n_raw, rel_e_raw),
            bearing_out=relative_bearing(rel_n, rel_e),
            distance_m=math.hypot(rel_n, rel_e),
        ))
        lines.append(pflaa(p, rel_n, rel_e, rel_v))

    lines.append(pflau(len(traffic)))
    if debug is not None:
        debug.log_build(snap)
    return lines


def push(host: str, port: int, interval: float, get_lines) -> None:
    """Connect to a TCP listener and send NMEA at a fixed rate."""
    print(f"Pushing to {host}:{port} every {interval:.0f}s",
          file=sys.stderr)

    while True:
        sock = None
        try:
            sock = socket.create_connection((host, port), timeout=5.0)
            out = sock.makefile("wb")
            print(f"Connected to {host}:{port}", file=sys.stderr)
            while True:
                tick = time.monotonic()
                for line in get_lines():
                    out.write(line.encode("ascii"))
                    out.write(b"\r\n")
                out.flush()
                time.sleep(max(0.0, interval - (time.monotonic() - tick)))
        except (ConnectionRefusedError, TimeoutError, OSError) as exc:
            print(f"Connect failed: {exc}", file=sys.stderr)
            time.sleep(interval)
        except (BrokenPipeError, ConnectionResetError):
            print("Disconnected, reconnecting…", file=sys.stderr)
            time.sleep(1.0)
        finally:
            if sock is not None:
                sock.close()


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Condor Spectate.json → FLARM PFLAA for XCSoar")
    src = ap.add_mutually_exclusive_group()
    src.add_argument("--file", type=Path,
                     help="Local Spectate.json path")
    src.add_argument("--smb", metavar="HOST",
                     help="SMB host (share name: Logs)")
    ap.add_argument("--share", default="Logs", help="SMB share (default: Logs)")
    ap.add_argument("--remote", default="Spectate.json",
                    help="Remote JSON file name")
    ap.add_argument("--smb-user", default="c3sim")
    ap.add_argument("--smb-pass", default="c3sim")
    ap.add_argument("--ref-id", metavar="ID",
                    help="Own-ship Condor player ID: relative coords are "
                    "computed from this glider and it is omitted from traffic")
    ap.add_argument("--own-cn", metavar="CN",
                    help="Own-ship competition number (alternative to --ref-id)")
    ap.add_argument("--host", default="127.0.0.1",
                    help="TCP destination (default: 127.0.0.1)")
    ap.add_argument("--port", type=int, default=2000,
                    help="TCP destination port (default: 2000)")
    ap.add_argument("--interval", type=float, default=1.0,
                    help="Spectate.json poll interval in seconds (default: 1)")
    ap.add_argument("--smooth", type=float, default=3.0, metavar="SEC",
                    help="EMA time constant for relative positions; 0=off "
                    "(default: 3)")
    ap.add_argument("--udp-port", type=int, metavar="PORT",
                    help="Listen for Condor UDP own-ship lat/lon (same port as "
                    "Condor3UDP; needs SO_REUSEPORT on Linux)")
    ap.add_argument("--log", type=Path, metavar="FILE",
                    help="Append debug log (ref frame, raw/smooth rel, FLIP lines)")
    ap.add_argument("--once", action="store_true",
                    help="Print sentences to stdout and exit")
    args = ap.parse_args()

    debug = DebugLogger(args.log) if args.log else None
    if debug is not None:
        print(f"Debug log: {args.log}", file=sys.stderr)

    own_pos = OwnShipPosition() if args.udp_port else None
    if args.udp_port:
        start_udp_listener(args.udp_port, own_pos)
        print(f"Listening for Condor UDP on port {args.udp_port}",
              file=sys.stderr)

    smoother = RelativeSmoother(args.smooth)

    def make_sentences(players: list[dict[str, Any]]) -> list[str]:
        return build_sentences(players, args.ref_id, args.own_cn,
                               own_pos, smoother, debug=debug)

    def read_players() -> list[dict[str, Any]]:
        if args.file:
            return load_spectate_file(args.file)
        if args.smb:
            return load_spectate_bytes(read_smb(args.smb, args.share,
                                                args.smb_user, args.smb_pass,
                                                args.remote))
        path = Path("/mnt/c3sim-logs/Spectate.json")
        if not path.is_file():
            raise SystemExit("Use --file or --smb (or mount Spectate.json "
                             "at /mnt/c3sim-logs/Spectate.json)")
        return load_spectate_file(path)

    if args.once:
        for line in make_sentences(read_players()):
            print(line)
        return

    def get_lines() -> list[str]:
        try:
            return make_sentences(read_players())
        except Exception as exc:
            print(f"Spectate read failed: {exc}", file=sys.stderr)
            if debug is not None:
                debug.write(f"{DebugLogger._stamp()} ERROR {exc}")
            return [pflau(0)]

    push(args.host, args.port, args.interval, get_lines)


if __name__ == "__main__":
    main()
