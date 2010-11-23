import sys
import os
from map_generator import MapGenerator
from angle import Angle
import shutil
import zipfile

def read_template_from_map(file):
    map = {}

    print "Opening map file \"" + file + "\" ..."
    zip = zipfile.ZipFile(file, "r")
    f = zip.open("INFO.TXT", "r")
    for line in f:
        line = line.strip()
        if line == "" or line[0] == "#":
            continue
        
        if line.upper().startswith("MAPSET NAME:"):
            # Mapset name: <AFR_GARIEP_DAM> srcbase: <soa>
            line = line.replace("Mapset name: ", "")
            line = line.strip().lstrip("<")
            line = line[:line.find(">")]
            map["NAME"] = line
            continue
    
        if line.upper().startswith("BOUNDARIES:"):
            # Boundaries: long. 19.3-31 lat. -34--24.30
            line = line.replace("Boundaries: ", "").strip()
            line = line.split(" ", 3)
            if len(line) != 4:
                return {}
            
            lons = ""
            lats = ""
            if line[0] == "long.":
                lons = line[1]
            if line[0] == "lat.":
                lats = line[1]
            if line[2] == "long.":
                lons = line[3]
            if line[2] == "lat.":
                lats = line[3]
            
            if lons == "" or lats == "":
                continue
            
            map["LONMIN"] = lons[:lons.find("-", 1)].strip()
            map["LONMAX"] = lons[lons.find("-", 1)+1:].strip()
            
            map["LATMIN"] = lats[:lats.find("-", 1)].strip()
            map["LATMAX"] = lats[lats.find("-", 1)+1:].strip()
            continue
    
    return map

def read_template_file(file):
    map = {}
    type = None

    print "Opening template file \"" + file + "\" ..."
    f = open(file, "r")
    for line in f:
        if type == None: 
            type = 1
            if line.startswith("CREATED BY"):
                type = 2
                continue
            
        line = line.strip()
        if line == "" or line[0] == "#":
            continue
        
        if type == 1:
            line = line.strip().upper().split("=", 1)
            if len(line) == 2:
                map[line[0]] = line[1]
                
        if type == 2:
            if line.upper().startswith("MAPSET NAME:"):
                # MAPSET NAME: <ALPS>
                line = line.replace("MAPSET NAME: ", "")
                line = line.strip().lstrip("<").rstrip(">")
                map["NAME"] = line
                continue
        
            line = line.replace("BOUNDARIES", "").strip()
            
            if line.startswith(": Longitude"):
                # : Longitude 4.5  to  16.5
                line = line.replace(": Longitude", "").strip()
                line = line.split(" to ", 1)
                if len(line) != 2:
                    return None
                
                map["LONMIN"] = line[0].strip()
                map["LONMAX"] = line[1].strip()
                continue
        
            if line.startswith(": Latitude"):
                # : Latitude 4.5  to  16.5
                line = line.replace(": Latitude", "").strip()
                line = line.split(" to ", 1)
                if len(line) != 2:
                    return None
                
                map["LATMIN"] = line[0].strip()
                map["LATMAX"] = line[1].strip()
                continue
        
    return map

def read_template(file):
    if file.lower().endswith(".lkm"):
        return read_template_from_map(file)
    elif file.lower().endswith(".txt"):
        return read_template_file(file)
    
    return None

def convert(template, working_dir):
    if not "NAME" in template:
        print "Template file has no NAME specified!"
        return
    
    name = template["NAME"]

    lkm_file = os.path.join(working_dir, name + ".LKM")
    xcm_file = os.path.join(working_dir, name + ".xcm")

    if os.path.exists(xcm_file):
        os.unlink(xcm_file)
        
    if not os.path.exists(lkm_file):
        print "LKM file \"" + lkm_file + "\" does not exist!"
        return
    
    shutil.copy(lkm_file, xcm_file)

    m = MapGenerator()
    m.SetBoundsSeperatly(Angle.degrees(float(template["LATMIN"].replace(",", "."))), 
                         Angle.degrees(float(template["LATMAX"].replace(",", "."))), 
                         Angle.degrees(float(template["LONMIN"].replace(",", "."))), 
                         Angle.degrees(float(template["LONMAX"].replace(",", "."))))
    m.AddTerrain(9)
    m.Create(xcm_file, True)

def main():
    if len(sys.argv) < 2:
        print "Too few arguments given! Please provide a map or template file."
        return
    
    file = sys.argv[1]
    if not os.path.exists(file):
        print "Nap or template file \"" + file + "\" does not exist!"
        return
    
    working_dir = os.path.dirname(os.path.abspath(file))
    template = read_template(file)
    if template != None:
        convert(template, working_dir)
    
if __name__ == '__main__':
    main()    
