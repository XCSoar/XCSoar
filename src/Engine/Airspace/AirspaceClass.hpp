// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum AirspaceClass : uint8_t
{
  OTHER = 0,
  RESTRICT,
  PROHIBITED,
  DANGER,
  CLASSA,
  CLASSB,
  CLASSC,
  CLASSD,
  NOGLIDER,
  CTR,
  WAVE,
  AATASK,
  CLASSE,
  CLASSF,
  TMZ,
  CLASSG,
  MATZ,
  RMZ,
  /*
   * There is no 'authoritative' documentation for the classes and their textual representation below.
   * The information is taken from the following sources:
   *
   * http://www.winpilot.com/UsersGuide/UserAirspace.asp
   *   AY = Airspace Type (extends AC which should now only be used for class)
   *
   * https://www.openaip.net/docs
   *   AY: Specifies the type of the airspace, e.g. "CTR". In the extended format, the AC tag is exclusively used to
   *   specify the airspace ICAO class. The AY tag is required and must be placed directly after the AC tag.
   *
   * From comments in the header of some airspace files (ex. https://storage.googleapis.com/29f98e10-a489-4c82-ae5e-489dbcd4912f/de_asp_extended.txt):
   *    "
   *    This is OpenAIR file is an extended format derived from the original specification. It adds additional fields:
   *      - AY: The AY field contains the type and the AC field is now exclusively used to specify the ICAO class of the defined airspace.
   *      - AF: The AF field contains contact frequencies defined for the airspace. If no frequency is defined, this field does not exist.
   *      - AG: The AG field contains the ground station call-sign related to the frequency defined for the airspace. If no frequency
   *           is defined, this field does not exist.
   *    This OpenAIR file does contain non-standard AY values. The following list shows all AY types that may be used within this file:
   *      UNCLASSIFIED, RESTRICTED, DANGER, PROHIBITED, CTR, TMZ, RMZ, TMA, TRA, TSA, FIR, UIR, ADIZ, ATZ, MATZ, AWY, MTR,
   *      ALERT, WARNING, PROTECTED, HTZ, GLIDING_SECTOR, TRP, TIZ, TIA, MTA, CTA, ACC_SECTOR, AERIAL_SPORTING_RECREATIONAL,
   *      OVERFLIGHT_RESTRICTION, MRT, TFR, VFR_SECTOR
   *
   *    This OpenAIR file does contain non-standard AC values. The following list shows all AC types that may be used within this file:
   *      A, B, C, D, E, F, G, UNCLASSIFIED
   *    "
   */
  UNCLASSIFIED,
  RESTRICTED,
  TMA,
  TRA,
  TSA,
  FIR,
  UIR,
  ADIZ,
  ATZ,
  AWY,
  MTR,
  ALERT,
  WARNING,
  PROTECTED,
  HTZ,
  GLIDING_SECTOR,
  TRP,
  TIZ,
  TIA,
  MTA,
  CTA,
  ACC_SECTOR,
  AERIAL_SPORTING_RECREATIONAL,
  OVERFLIGHT_RESTRICTION,
  MRT,
  TFR ,
  VFR_SECTOR,
  AIRSPACECLASSCOUNT
};
