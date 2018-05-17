/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "IGC/IGCWriter.hpp"
#include "NMEA/GPSState.hpp"

/*
 * From FAI_Tech_Spec_Gnss.pdf
 * EPE - Estimated Position Error - An estimate by a GNSS receiver of the probability of position error in each fix,
 * taking into account the geometry factors of DOP (qv above) with the addition of factors such as received signal
 * strength. The probability used in the calculation should be stated so that the significance of the size of the
 * resulting shape (frequently a circular error) is known. Probabilities are frequently calculated to a 2-sigma
 * (95.45%) level, implying that there is about a 95% (19 out of 20) chance that the true position is inside the shape
 * concerned. This probability figure applies to a single fix in isolation and is increased by taking into account
 * adjacent fixes and with knowledge of how gliders are flown. The EPE value appears in the IGC file as a three
 * number group in metres through the FXA code. (AL3)
 *
 * DOP - Dilution of Precision - The reduction of precision in a GNSS fix due to the geometry of the constellation
 * (of satellites used for the fix. Computed by a GNSS receiver for each fix, see also Estimated Position Error
 * (EPE). DOP can be estimated in various ways, including, HDOP (Horizontal position), GDOP (Geometric),
 * PDOP (Position, overall), TDOP (Time) and VDOP (Vertical position). EPE also varies with constellation
 * position. Some definitions from RTCA sources are given below (RTCA = the US Radio Technical Commission
 * for Aeronautics). (AL5)
 *
 * Horizontal fix accuracy - the best prediction for the horizontal 2-sigma (95.45% probability) error of the overall
 * position error. Included in the IGC data file in the B (fix) record through the FXA three-letter code. (AL4)
 */

const char *
IGCWriter::GetHFFXARecord()
{
  /*
   * HFFXA Record
   * From FAI_Tech_Spec_Gnss.pdf
   * Fix Data Accuracy Category. When used in the header record, this is a general
   * indication of potential fix accuracy and indicates a category of receiver capability
   * rather than an exact figure such as applies to each recorded fix in the B, I, J or K
   * records, see above. If in doubt, use a three figure group in metres that refers to a
   * typical EPE radius achieved by the receiver in good reception conditions. (AL3)
   *
   * XCSoar Interpretation: Use 50 meters to be conservative:
   * ---> HFFXA050
   */
  return "HFFXA050";
}

const char *
IGCWriter::GetIRecord()
{
  /*
   * I Record
   * From FAI_Tech_Spec_Gnss.pdf
   * 3.4 I RECORD - EXTENSIONS TO THE FIX (B) RECORD. The I record defines any extensions to the fix
   * (B) Record in the form of a list of the appropriate Three-Letter Codes (CCC), data for which will appear at the
   * end of subsequent B Records. Only one I-Record line is included in each file, located after the H record and
   * before the first B Record. The Fix Accuracy (FXA) must be included, in the form of the Estimated Position
   * Error figure (see Glossary under EPE). This shall be followed by SIU, ENL and RPM, if these are recorded in
   * the FR concerned. Note that although the SIU number is optional in the B record, the F Record (satellite
   * constellation used) is mandatory, see para 4.3. The format of the I Record is as follows: (AL11)
   *
   * XCSoar Interpretation:
   *
   * XCSoar appends the EPE and the SIU to each B Record
   * I 02 3638 FXA 3940 SIU (contextual spaces shown)
   *
   *    2 parameters appended to B record:
   *    Param 1 = FXA from bytes 36-38.
   *    Param 2 = SIU from bytes 39-40
   *
   * ---> I023638FXA3940SIU  (no spaces)
   */
  return "I023638FXA3940SIU";
}

double
IGCWriter::GetEPE(const GPSState &gps)
{
  /*
   * EPE (Estimated Position Error in meters)
   * based on Parkinson, Bradford W. Global Positioning System Theory and Applications Volume 1.
   * URE (User Range Error) is an estimate of "Signals in Space" errors, i.e., ephemeris data,
   * satellite clocks, ionospheric delay and tropospheric delay. These errors can be greatly
   * reduced by differential and multiple frequency techniques. Differential correction sources
   * include user provided reference stations, community base stations, governmental beacon
   * transmissions, FM sub-carrier transmissions and geosynchronous satellite transmissions.

   * UEE (User Equipment Errors) includes receiver noise, multipath, antenna orientation,
   * EMI/RFI. Receiver and antenna design can greatly reduce UEE error sources--usually at
   * substantial cost.
   *
   * Typical Error components of an autonomous (non Differential) GPS signal are:
   *    Error           Error in meters   (autonomous)  DGPS      Type error
   *    Ephemeris data                      2.1         0         URE
   *    Satellite clock                     2.1         0         URE
   *    Ionosphere                          4           0.4       URE
   *    Troposphere                         0.7         0.2       URE

   *    Multipath                           1.4         1.4       UEE
   *    Receiver measurement                0.5         0.5       UEE
   *
   * Autonomous:
   * EPE 2-sigma = EPE (2drms) = 2* HDOP * SQRT [URE^2 + UEE^2] = HDOP * 18.2
   *
   * Differential:
   * EPE 2-sigma = EPE (2drms) = 2* HDOP * SQRT [URE^2 + UEE^2] = HDOP * 4.0

   * Assumptions: XCSoar does not know any details about the hardware of the receiver,
   * so we assume the table above is a valid estimation.
   *
   * The WAAS function in the US does not set the DGPS flag
   * fix_quality
   * 1 = GPS fix (SPS)
   * 2 = DGPS fix
   * 3 = PPS fix
   * 4 = Real Time Kinematic
   * 5 = Float RTK
   * 6 = estimated (dead reckoning) (2.3 feature)
   * 7 = Manual input mode
   * 8 = Simulation mode
   */

  switch (gps.fix_quality) {
  case FixQuality::GPS:
    return gps.hdop * 18.2;

  case FixQuality::DGPS:
    return gps.hdop * 4;

  default:
    return 0;
  }
}

int
IGCWriter::GetSIU(const GPSState &gps)
{
  switch (gps.fix_quality) {
  case FixQuality::GPS:
  case FixQuality::DGPS:
    if (gps.satellites_used_available)
      return gps.satellites_used;

  default:
    break;
  }

  return 0;
}
