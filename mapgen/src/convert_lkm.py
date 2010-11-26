import sys
import os
from map_generator import MapGenerator
from angle import Angle
import shutil
import zipfile
from tempfile import mkdtemp

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

def update_topology_file(temp_dir):
    old_file = os.path.join(temp_dir, "topology_old.tpl")
    new_file = os.path.join(temp_dir, "topology.tpl")
    os.rename(new_file, old_file)
    
    old = open(old_file, "r")
    new = open(new_file, "w")
    for line in old:
        if line.lower().startswith("coast_area"): continue
        new.write(line)
    
    old.close()
    new.close()
    
    os.unlink(old_file)

def convert(template, working_dir):
    if not "NAME" in template:
        print "Template file has no NAME specified!"
        return
    
    if ((not "LATMIN" in template) or (not "LATMAX" in template) or
        (not "LONMIN" in template) or (not "LONMIN" in template)):
        print "Template file has no bounds specified!"
        return
    
    name = template["NAME"]

    lkm_file = os.path.join(working_dir, name + ".LKM")
    xcm_file = os.path.join(working_dir, name + ".xcm")

    if not os.path.exists(lkm_file):
        print "LKM file \"" + lkm_file + "\" does not exist!"
        return
    
    m = MapGenerator()
    m.SetBoundsSeperatly(Angle.degrees(float(template["LATMIN"].replace(",", "."))), 
                         Angle.degrees(float(template["LATMAX"].replace(",", "."))), 
                         Angle.degrees(float(template["LONMIN"].replace(",", "."))), 
                         Angle.degrees(float(template["LONMAX"].replace(",", "."))))
    m.AddTerrain(9)

    lkm = zipfile.ZipFile(lkm_file, "r");
    needed_files = []
    for file in lkm.namelist():
        # Don't add coast_area shapefile to the XCM file
        if file.lower().startswith("coast_area."): continue
        # Don't add info.txt to the XCM file
        # -> we will create our own...
        if file.lower() == "info.txt": continue
        needed_files.append(file)
        
    if needed_files == []:
        print "LKM file \"" + lkm_file + "\" is empty!"
        return
    
    # Create temporary folder
    temp_dir = mkdtemp()
    # Extract LKM contents to temporary folder
    print "Extracting \"" + lkm_file + "\" ..."
    lkm.extractall(temp_dir, needed_files)
    lkm.close()
    
    # Remove coast_area shapefile from topology file
    update_topology_file(temp_dir)

    # Delete old XCM file if exists
    if os.path.exists(xcm_file):
        os.unlink(xcm_file)
        
    # Create new XCM file
    print "Creating \"" + xcm_file + "\" ..."
    xcm = zipfile.ZipFile(xcm_file, "w");
    for file in needed_files:
        compress = zipfile.ZIP_DEFLATED
        # Don't compress shapefiles
        if ((file.lower().endswith(".dbf")) or 
            (file.lower().endswith(".prj")) or
            (file.lower().endswith(".shp")) or 
            (file.lower().endswith(".shx"))): 
            compress = zipfile.ZIP_STORED
            
        xcm.write(os.path.join(temp_dir, file), file, compress)
    xcm.close()
    
    # Delete temporary files
    print "Deleting temporary files ..."
    for file in needed_files:
        os.unlink(os.path.join(temp_dir, file))
    os.rmdir(temp_dir)
     
    # Add terrain to XCM file
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
