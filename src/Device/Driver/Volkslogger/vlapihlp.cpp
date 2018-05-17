/***********************************************************************
**
**   vlapihlp.cpp
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include "vlapihlp.h"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"

#include <string.h>
#include <stdio.h>
#include <math.h>

gcc_const
static bool
IsAllowedIGCChar(char ch)
{
  static constexpr char alphabet[] =
    " \"#%&\'()+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_\140abcdefghijklmnopqrstuvwxyz{|}";
  static constexpr size_t alphabet_l = ARRAY_SIZE(alphabet) - 1;

  return memchr(alphabet, ch, alphabet_l) != nullptr;
}

/*
Filtern einer Zeile:
  - Umwandeln von nicht-IGC-Zeichen in Leerzeichen
  - Entfernen von Leer- und Sonderzeichen am Ende
*/
char *igc_filter(char *st) {
  for (char *p = st; *p != 0; ++p)
    if (!IsAllowedIGCChar(*p))
      *p = ' ';
  StripRight(st);
  return st;
}

// Aus einer 2byte-Binärzahl einen base-36 Seriennummernstring machen
void
wordtoserno(char *Seriennummer, unsigned Binaer)
{
  static constexpr char base36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  Seriennummer[0] = base36[(Binaer / 36 / 36) % 36];
  Seriennummer[1] = base36[(Binaer / 36) % 36];
  Seriennummer[2] = base36[Binaer % 36];
  Seriennummer[3] = 0;
}



/*
genaue Umrechnung von Druckwert nach Höhe.
Druckwert ist ein innerhalb des Loggers temperaturkompensierter Wert, der
proportional zum gemessenen Umgebungsdruck am Logger ist.
1100 mbar entsprechen einem Druckwert von 4096;
*/
long pressure2altitude(unsigned druck) {
double GMR   = 9.80665*28.9644/8314.32;
double tgrad = -6.5E-3;
double p0    = 1013.25;
double p11   = 0.2233611050922 * p0;
double t0    = 288.15;
double t11   = 216.65;
double p;
double hoehe;
 // Umrechnung von normierten ADC-Werten in hPa
 p = 1100.0*druck/4096;
 // Berechnen der Druckhöhe anhand des Druckes
 if (p>p11)
   hoehe = (t0 * (exp((tgrad/GMR)*log(p0/p)) - 1) / tgrad);
 else
   hoehe = (t11*log(p11/p)/GMR + 11000);
 return (long int) hoehe;
}



