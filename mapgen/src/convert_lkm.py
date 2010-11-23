import sys
import os
from map_generator import MapGenerator
from angle import Angle
import shutil

def read_template(file):
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
            if line.startswith("MAPSET NAME:"):
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
                    return {}
                
                map["LONMIN"] = line[0].strip()
                map["LONMAX"] = line[1].strip()
                continue
        
            if line.startswith(": Latitude"):
                # : Latitude 4.5  to  16.5
                line = line.replace(": Latitude", "").strip()
                line = line.split(" to ", 1)
                if len(line) != 2:
                    return {}
                
                map["LATMIN"] = line[0].strip()
                map["LATMAX"] = line[1].strip()
                continue
        
    return map

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
        print "Too few arguments given! Please provide a template file."
        return
    
    file = sys.argv[1]
    if not os.path.exists(file):
        print "Template file \"" + file + "\" does not exist!"
        return
    
    working_dir = os.path.dirname(os.path.abspath(file))
    template = read_template(file)
    convert(template, working_dir)
    
if __name__ == '__main__':
    main()    
