/*
 * XCSoar Glide Computer - http://www.xcsoar.org/
 * Copyright (C) 2000-2015 The XCSoar Project
 * A detailed list of copyright holders can be found in the file "AUTHORS".
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

#ifndef XCSOAR_TRACKING_SKYLINES_PROTOCOL_HPP
#define XCSOAR_TRACKING_SKYLINES_PROTOCOL_HPP

#include <stdint.h>

/*
 * This file defines the SkyLines live tracking protocol.  It is a
 * one-way datagram protocol: the client (i.e. the on-board navigation
 * device) sends datagrams to the server without expecting a response.
 * It is light-weight.
 *
 * Mobile network availability is said to be bad during a flight,
 * which directed the design of this protocol.  The submission
 * frequency is high enough that useful data will be seen on the
 * server even if packet loss is high.  The protocol does not require
 * a handshake, because that would raise the requirements for network
 * reliability.
 *
 */

/*
 * The struct definitions below imply a specific memory layout.  They
 * have been designed in a way that all compilers we know will not
 * implicitly insert padding, because all attributes are aligned
 * properly already.
 *
 * All integers are big-endian.
 *
 */

namespace SkyLinesTracking {

static const uint32_t MAGIC = 0x5df4b67b;

enum Type {
  PING = 1,
  ACK = 2,
  FIX = 3,
  TRAFFIC_REQUEST = 4,
  TRAFFIC_RESPONSE = 5,
  USER_NAME_REQUEST = 6,
  USER_NAME_RESPONSE = 7,

  /**
   * @see #WaveSubmitPacket
   */
  WAVE_SUBMIT = 8,

  /**
   * @see #WaveRequestPacket
   */
  WAVE_REQUEST = 9,

  /**
   * @see #WaveResponsePacket
   */
  WAVE_RESPONSE = 10,

  /**
   * @see #ThermalSubmitPacket
   */
  THERMAL_SUBMIT = 11,

  /**
   * @see #ThermalRequestPacket
   */
  THERMAL_REQUEST = 12,

  /**
   * @see #ThermalResponsePacket
   */
  THERMAL_RESPONSE = 13,
};

/**
 * The datagram payload header.
 */
struct Header {
  /**
   * Must be MAGIC.
   */
  uint32_t magic;

  /**
   * The CRC of this packet including the header, assuming this
   * attribute is 0.
   *
   * The CRC algorithm is CRC16-CCITT with initial value 0x0000
   * (XModem) instead of CCITT's default 0xffff.
   */
  uint16_t crc;

  /**
   * An "enum Type" value.
   */
  uint16_t type;

  /**
   * The authorization key.
   */
  uint64_t key;
};

#ifdef __cplusplus
static_assert(sizeof(Header) == 16, "Wrong struct size");
#endif

/**
 * Check the network connection and verify the key (#PING).  The
 * server responds with #ACK.
 */
struct PingPacket {
  Header header;

  /**
   * An arbitrary number chosen by the client, usually a sequence
   * number.
   */
  uint16_t id;

  /**
   * Reserved for future use.  Set to zero.
   */
  uint16_t reserved;

  /**
   * Reserved for future use.  Set to zero.
   */
  uint32_t reserved2;
};

#ifdef __cplusplus
static_assert(sizeof(PingPacket) == 24, "Wrong struct size");
#endif

/**
 * A generic acknowledge packet sent by the server in response to
 * certain request packets.
 */
struct ACKPacket {
  /**
   * The key was not valid.  Usually, requests with bad keys are
   * silently discarded, but the server may use this flag to respond
   * to a bad key in a PING packet.
   */
  static const uint32_t FLAG_BAD_KEY = 0x1;

  Header header;

  /**
   * Copy of the request's id value.
   */
  uint16_t id;

  /**
   * Reserved for future use.  Set to zero.
   */
  uint16_t reserved;

  uint32_t flags;
};

#ifdef __cplusplus
static_assert(sizeof(ACKPacket) == 24, "Wrong struct size");
#endif

struct GeoPoint {
  /**
   * Angle in micro degrees.  Positive means north or east.
   */
  int32_t latitude, longitude;
};

#ifdef __cplusplus
static_assert(sizeof(GeoPoint) == 8, "Wrong struct size");
#endif

/**
 * A GPS fix being uploaded to the server.
 */
struct FixPacket {
  static const uint32_t FLAG_LOCATION = 0x1;
  static const uint32_t FLAG_TRACK = 0x2;
  static const uint32_t FLAG_GROUND_SPEED = 0x4;
  static const uint32_t FLAG_AIRSPEED = 0x8;
  static const uint32_t FLAG_ALTITUDE = 0x10;
  static const uint32_t FLAG_VARIO = 0x20;
  static const uint32_t FLAG_ENL = 0x40;

  Header header;

  uint32_t flags;

  /**
   * Millisecond of day (UTC).  May be bigger than 24*60*60*1000 if
   * the flight has wrapped midnight.
   */
  uint32_t time;

  GeoPoint location;

  /**
   * Reserved for future use.  Set to zero.
   */
  uint32_t reserved;

  /**
   * Ground track in degrees (0..359).
   */
  uint16_t track;

  /**
   * Ground speed in m/16s.
   */
  uint16_t ground_speed;

  /**
   * Indicated air speed in m/16s.
   */
  uint16_t airspeed;

  /**
   * Altitude in m above MSL.
   */
  int16_t altitude;

  /**
   * Vertical speed in m/256s.
   */
  int16_t vario;

  /**
   * Engine noise level value from the logger, valid range is
   * 0..999.
   */
  uint16_t engine_noise_level;
};

#ifdef __cplusplus
static_assert(sizeof(FixPacket) == 48, "Wrong struct size");
#endif

/**
 * The client requests traffic information.
 */
struct TrafficRequestPacket {
  /**
   * The client wants to receive information about all pilots he
   * follows on the SkyLines web site.
   */
  static const uint32_t FLAG_FOLLOWEES = 0x1;

  /**
   * The client wants to receive information about all members in
   * the same club.
   */
  static const uint32_t FLAG_CLUB = 0x2;

  /**
   * The client wants to receive information about all traffic near
   * the location he submitted recently in a #FixPacket.  The server
   * chooses a reasonable range.
   */
  static const uint32_t FLAG_NEAR = 0x4;

  Header header;

  uint32_t flags;

  uint32_t reserved;
};

#ifdef __cplusplus
static_assert(sizeof(TrafficRequestPacket) == 24, "Wrong struct size");
#endif

/**
 * The responds to #TrafficRequestPacket.  This packet has a dynamic
 * length.  If there are many records being sent, the packet should
 * be split at a reasonable size, to avoid implicit UDP datagram
 * fragmentation.
 */
struct TrafficResponsePacket {
  struct Traffic {
    uint32_t pilot_id;

    /**
     * Millisecond of day (UTC).  This is the time this information
     * was submitted to SkyLines by the pilot described in this
     * object.
     */
    uint32_t time;

    GeoPoint location;

    int16_t altitude;

    /**
     * Reserved for future use.
     */
    uint16_t reserved;

    /**
     * Reserved for future use.
     */
    uint32_t reserved2;
  };

#ifdef __cplusplus
  static_assert(sizeof(Traffic) == 24, "Wrong struct size");
#endif

  Header header;

  /**
   * Reserved for future use.
   */
  uint16_t reserved;

  /**
   * Reserved for future use.
   */
  uint8_t reserved2;

  /**
   * The number of #Traffic instances following this struct.
   */
  uint8_t traffic_count;

  /**
   * Reserved for future use.
   */
  uint32_t reserved3;

  /* followed by a number of #Traffic instances */
};

#ifdef __cplusplus
static_assert(sizeof(TrafficRequestPacket) == 24, "Wrong struct size");
#endif

/**
 * The client requests the name of a user.
 */
struct UserNameRequestPacket {
  Header header;

  /**
   * The id of the user, as obtained by
   * #TrafficResponsePacket::Packet::pilot_id.
   */
  uint32_t user_id;

  /**
   * Reserved for future use.
   */
  uint32_t reserved;
};

#ifdef __cplusplus
static_assert(sizeof(UserNameRequestPacket) == 24, "Wrong struct size");
#endif

/**
 * The server replies with information about a user.
 */
struct UserNameResponsePacket {
  /**
   * A user with the specified id was not found.  The following
   * attributes are undefined.
   */
  static const uint32_t FLAG_NOT_FOUND = 0x1;

  Header header;

  /**
   * The id of the user, as obtained by
   * #TrafficResponsePacket::Packet::pilot_id.
   */
  uint32_t user_id;

  uint32_t flags;

  /**
   * The club the user belongs to.  0 means no club.
   */
  uint32_t club_id;

  /**
   * The name of the user in UTF-8 bytes.
   */
  uint8_t name_length;

  /**
   * Reserved for future use.
   */
  uint8_t reserved1, reserved2, reserved3;
  uint32_t reserved4, reserved5;

  /* the struct is followed by #name_length bytes of UTF-8
     containing the name, not null-terminated */
};

#ifdef __cplusplus
static_assert(sizeof(UserNameResponsePacket) == 40, "Wrong struct size");
#endif

/**
 * Packet fragment which describes one wave.
 */
struct Wave {
  /**
   * Millisecond of day (UTC).  This is the time this wave was last
   * seen.
   */
  uint32_t time;

  /**
   * This reserved field may one day become the reporter's user id.
   */
  uint32_t reserved1;

  /**
   * Two points describing the wave axis where lift was found.
   */
  GeoPoint a, b;

  /**
   * Approximate bottom altitude where this wave was found.  For
   * example, this may be the altitude where the glider entered the
   * wave.
   *
   * Note: this is unused currently.
   */
  int16_t bottom_altitude;

  /**
   * Approximate top altitude where this wave was found.  For
   * example, this may be the altitude where the glider left the
   * wave.
   *
   * Note: this is unused currently.
   */
  int16_t top_altitude;

  /**
   * Average lift m/256s.
   *
   * Note: this is unused currently.
   */
  uint16_t lift;

  int16_t reserved2;
};

/**
 * The client submits the location of a wave he detected.
 */
struct WaveSubmitPacket {
  Header header;

  Wave wave;
};

/**
 * The client wishes to receive wave information.  The server will
 * send #WAVE_RESPONSE / #WaveResponsePacket.
 */
struct WaveRequestPacket {
  Header header;

  /**
   * Unused.
   */
  uint32_t flags;

  uint32_t reserved1;
};

/**
 * Reply to #WAVE_REQUEST / #WaveRequestPacket.
 */
struct WaveResponsePacket {
  Header header;

  uint16_t reserved1;
  uint8_t reserved2;

  /**
   * The number of #Wave instances following this struct.
   */
  uint8_t wave_count;

  uint32_t reserved3;

  /* followed by a number of #Wave instances */
};

/**
 * Packet fragment which describes one thermal.
 */
struct Thermal {
  /**
   * Millisecond of day (UTC).  This is the time this thermal was
   * last seen.
   */
  uint32_t time;

  /**
   * This reserved field may one day become the reporter's user id.
   */
  uint32_t reserved1;

  /**
   * The location of the glider at its bottom-most altitude.
   * Usually, this is where the glider entered the thermal.
   */
  GeoPoint bottom_location;

  /**
   * The location of the glider at its top-most altitude.
   * Usually, this is where the glider left the thermal.
   */
  GeoPoint top_location;

  /**
   * The bottom-most aircraft altitude inside this thermal.
   */
  int16_t bottom_altitude;

  /**
   * The top-most aircraft altitude inside this thermal.
   */
  int16_t top_altitude;

  /**
   * Average lift [m/256s].
   */
  uint16_t lift;

  int16_t reserved2;
};

/**
 * The client submits the location of a thermal he detected.
 */
struct ThermalSubmitPacket {
  Header header;

  Thermal thermal;
};

/**
 * The client wishes to receive thermal information.  The server will
 * send #THERMAL_RESPONSE / #ThermalResponsePacket.
 */
struct ThermalRequestPacket {
  Header header;

  /**
   * Unused.
   */
  uint32_t flags;

  uint32_t reserved1;
};

/**
 * Reply to #THERMAL_REQUEST / #ThermalRequestPacket.
 */
struct ThermalResponsePacket {
  Header header;

  uint16_t reserved1;
  uint8_t reserved2;

  /**
   * The number of #Thermal instances following this struct.
   */
  uint8_t thermal_count;

  uint32_t reserved3;

  /* followed by a number of #Thermal instances */
};

} /* namespace SkyLinesTracking */

#endif
