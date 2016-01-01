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

#include "LXN.hpp"

const struct LXN::ExtensionDefinition LXN::extension_defs[16] = {
    { "FXA", 3 },
    { "VXA", 3 },
    { "RPM", 5 },
    { "GSP", 5 },
    { "IAS", 5 },
    { "TAS", 5 },
    { "HDM", 3 },
    { "HDT", 3 },
    { "TRM", 3 },
    { "TRT", 3 },
    { "TEN", 5 },
    { "WDI", 3 },
    { "WVE", 5 },
    { "ENL", 3 },
    { "VAR", 3 },
    { "XX3", 3 }
};

const char *
LXN::FormatGPSDate(unsigned gps_date)
{
    static const char *const list[] = {
        "ADINDAN        ",
        "AFGOOYE        ",
        "AIN EL ABD 1970",
        "COCOS ISLAND   ",
        "ARC 1950       ",
        "ARC 1960       ",
        "ASCENSION 1958 ",
        "ASTRO BEACON E ",
        "AUSTRALIAN 1966",
        "AUSTRALIAN 1984",
        "ASTRO DOS 7/14 ",
        "MARCUS ISLAND  ",
        "TERN ISLAND    ",
        "BELLEVUE (IGN) ",
        "BERMUDA 1957   ",
        "COLOMBIA       ",
        "CAMPO INCHAUSPE",
        "CANTON ASTRO   ",
        "CAPE CANAVERAL ",
        "CAPE (AFRICA)  ",
        "CARTHAGE       ",
        "CHATHAM 1971   ",
        "CHUA ASTRO     ",
        "CORREGO ALEGRE ",
        "DJAKARTA       ",
        "DOS 1968       ",
        "EASTER ISLAND  ",
        "EUROPEAN 1950  ",
        "EUROPEAN 1979  ",
        "FINLAND 1910   ",
        "GANDAJIKA BASE ",
        "NEW ZEALAND '49",
        "OSGB 1936      ",
        "GUAM 1963      ",
        "GUX 1 ASTRO    ",
        "HJOESEY 1955   ",
        "HONG KONG 1962 ",
        "INDIAN/NEPAL   ",
        "INDIAN/VIETNAM ",
        "IRELAND 1965   ",
        "DIEGO GARCIA   ",
        "JOHNSTON 1961  ",
        "KANDAWALA      ",
        "KERGUELEN ISL. ",
        "KERTAU 1948    ",
        "CAYMAN BRAC    ",
        "LIBERIA 1964   ",
        "LUZON/MINDANAO ",
        "LUZON PHILIPPI.",
        "MAHE 1971      ",
        "MARCO ASTRO    ",
        "MASSAWA        ",
        "MERCHICH       ",
        "MIDWAY ASTRO'61",
        "MINNA (NIGERIA)",
        "NAD-1927 ALASKA",
        "NAD-1927 BAHAM.",
        "NAD-1927 CENTR.",
        "NAD-1927 CANAL ",
        "NAD-1927 CANADA",
        "NAD-1927 CARIB.",
        "NAD-1927 CONUS ",
        "NAD-1927 CUBA  ",
        "NAD-1927 GREEN.",
        "NAD-1927 MEXICO",
        "NAD-1927 SALVA.",
        "NAD-1983       ",
        "NAPARIMA       ",
        "MASIRAH ISLAND ",
        "SAUDI ARABIA   ",
        "ARAB EMIRATES  ",
        "OBSERVATORIO'66",
        "OLD EGYIPTIAN  ",
        "OLD HAWAIIAN   ",
        "OMAN           ",
        "CANARY ISLAND  ",
        "PICAIRN 1967   ",
        "PUERTO RICO    ",
        "QATAR NATIONAL ",
        "QORNOQ         ",
        "REUNION        ",
        "ROME 1940      ",
        "RT-90 SWEDEN   ",
        "S.AMERICA  1956",
        "S.AMERICA  1956",
        "SOUTH ASIA     ",
        "CHILEAN 1963   ",
        "SANTO(DOS)     ",
        "SAO BRAZ       ",
        "SAPPER HILL    ",
        "SCHWARZECK     ",
        "SOUTHEAST BASE ",
        "FAIAL          ",
        "TIMBALI 1948   ",
        "TOKYO          ",
        "TRISTAN ASTRO  ",
        "RESERVED       ",
        "VITI LEVU 1916 ",
        "WAKE-ENIWETOK  ",
        "WGS-1972       ",
        "WGS-1984       ",
        "ZANDERIJ       ",
        "CH-1903        "
    };

    if (gps_date < 103)
        return list[gps_date];
    else
        return "";
}

const char *
LXN::FormatCompetitionClass(unsigned class_id)
{
    static const char *const names[] = {
        "STANDARD",
        "15-METER",
        "OPEN",
        "18-METER",
        "WORLD",
        "DOUBLE",
        "MOTOR_GL",
    };

    return class_id < 7
      ? names[class_id]
      : "";
}
