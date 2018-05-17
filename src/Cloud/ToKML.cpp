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

#include "Data.hpp"
#include "Serialiser.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/FileReader.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "Util/PrintException.hxx"
#include "Compiler.h"

#include <iostream>

static constexpr std::chrono::steady_clock::duration MAX_TRAFFIC_AGE = std::chrono::hours(12);
static constexpr std::chrono::steady_clock::duration MAX_THERMAL_AGE = std::chrono::hours(12);

using std::cout;
using std::cerr;
using std::endl;

static inline void
ToKML(BufferedOutputStream &os, const AGeoPoint p)
{
  os.Format("<Point><coordinates>%f,%f,%f</coordinates></Point>",
            p.longitude.Degrees(),
            p.latitude.Degrees(),
            p.altitude);
}

gcc_unused
static inline void
ToKML(BufferedOutputStream &os, const GeoPoint p)
{
  ToKML(os, AGeoPoint(p, 0));
}

static void
ToKML(BufferedOutputStream &os, const CloudClient &client)
{
  os.Format("<Placemark>\n"
            "  <name>%u</name>\n"
            "  <ExtendedData>\n"
            "    <SchemaData schemaUrl=\"#traffic\">\n"
            "      <SimpleData name=\"id\">%u</SimpleData>\n"
            "    </SchemaData>\n"
            "  </ExtendedData>\n",
            client.id, client.id);
  ToKML(os, AGeoPoint(client.location, client.altitude));
  os.Write("</Placemark>\n");
}

static void
ToKML(BufferedOutputStream &os, const CloudClientContainer &clients)
{
  os.Write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
           "  <Document>\n"
           "    <Schema name=\"traffic\" id=\"traffic\">\n"
           "      <SimpleField name=\"lift\" type=\"float\"/>\n"
           "    </Schema>\n");
  os.Write("    <Folder>\n"
           "      <name>Traffic</name>\n");

  const auto min_stamp = std::chrono::steady_clock::now() - MAX_TRAFFIC_AGE;

  for (const auto &client : clients)
    if (client.stamp >= min_stamp)
      ToKML(os, client);

  os.Write("    </Folder>\n");
  os.Write("  </Document>\n"
           "</kml>");
}

static void
ToKML(BufferedOutputStream &os, const CloudThermal &thermal)
{
  os.Format("<Placemark>\n"
            "  <name>%f m/s</name>\n"
            "  <ExtendedData>\n"
            "    <SchemaData schemaUrl=\"#thermal\">\n"
            "      <SimpleData name=\"lift\">%f</SimpleData>\n"
            "    </SchemaData>\n"
            "  </ExtendedData>\n",
            thermal.lift, thermal.lift);
  ToKML(os, thermal.bottom_location);
  os.Write("</Placemark>\n");
}

static void
ToKML(BufferedOutputStream &os, const CloudThermalContainer &thermals)
{
  os.Write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
           "  <Document>\n"
           "    <Schema name=\"thermal\" id=\"thermal\">\n"
           "      <SimpleField name=\"id\" type=\"int\"/>\n"
           "    </Schema>\n");
  os.Write("    <Folder>\n"
           "      <name>Thermals</name>\n");

  const auto min_time = std::chrono::steady_clock::now() - MAX_THERMAL_AGE;

  for (const auto &thermal : thermals)
    if (thermal.time >= min_time)
      ToKML(os, thermal);

  os.Write("    </Folder>\n");
  os.Write("  </Document>\n"
           "</kml>");
}

gcc_unused
static void
ToKML(BufferedOutputStream &os, const CloudData &data)
{
  os.Write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
           "  <Document>\n"
           "    <Schema name=\"traffic\" id=\"traffic\">\n"
           "      <SimpleField name=\"lift\" type=\"float\"/>\n"
           "    </Schema>\n"
           "    <Schema name=\"thermal\" id=\"thermal\">\n"
           "      <SimpleField name=\"id\" type=\"int\"/>\n"
           "    </Schema>\n");
  ToKML(os, data.clients);
  ToKML(os, data.thermals);
  os.Write("  </Document>\n"
           "</kml>");
}

int
main(int argc, char **argv)
try {
  if (argc != 4) {
    cerr << "Usage: " << argv[0] << " DBPATH CLIENTS_KMLPATH THERMALS_KMLPATH" << endl;
    return EXIT_FAILURE;
  }

  const Path db_path(argv[1]);
  const Path clients_kml_path(argv[2]);
  const Path thermals_kml_path(argv[3]);

  CloudData data;

  /* read the database saved by xcsoar-cloud-server */

  {
    FileReader fr(db_path);
    Deserialiser s(fr);
    data.Load(s);
  }

  /* write the clients to KML */

  {
    FileOutputStream fos(clients_kml_path);

    {
      BufferedOutputStream bos(fos);
      ToKML(bos, data.clients);
      bos.Flush();
    }

    fos.Commit();
  }

  /* write the thermals to KML */

  {
    FileOutputStream fos(thermals_kml_path);

    {
      BufferedOutputStream bos(fos);
      ToKML(bos, data.thermals);
      bos.Flush();
    }

    fos.Commit();
  }

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
