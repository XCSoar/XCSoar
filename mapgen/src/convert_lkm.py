import sys
import os
from map_generator import MapGenerator
from angle import Angle
import shutil

def read_template(file):
    map = {}

    print "Opening template file \"" + file + "\" ..."
    f = open(file, "r")
    for line in f:
        if line[0] == "#":
            continue
        
        line = line.strip().upper().split("=", 1)
        if len(line) == 2:
            map[line[0]] = line[1]
        
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
    m.SetBoundsSeperatly(Angle.degrees(float(template["LATMIN"])), 
                         Angle.degrees(float(template["LATMAX"])), 
                         Angle.degrees(float(template["LONMIN"])), 
                         Angle.degrees(float(template["LONMAX"])))
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
