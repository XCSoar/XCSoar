import urllib
import os
import subprocess
import socket

from georect import GeoRect

cmd_ogr2ogr = "ogr2ogr"
cmd_7zip = "7za"
#gather_from_server = None
gather_from_server = "http://gis-lab.info/data/vmap0/"
#gather_from_server = "http://webbuster.dyndns.info/xcsoar/mapgen_data/topo/vmap0/"

__maps = ["eur", "noa", "sas", "soa"]
__layers = [["pop-miscellaneous-population-p", "mispopppop_point", "", "txt"],
            ["pop-built-up-a", "builtupapop_area", "", "nam"],
            ["hydro-water-course-l", "watrcrslhydro_line", "hyc=8", "nam"],
            ["hydro-inland-water-a", "inwaterahydro_area", "hyc=8", "nam"],
            ["trans-railroad-l", "railrdltrans_line", "exs=28", "fco"],
            ["trans-road-l", "roadltrans_line", "rtt=14", "med"]]

def __gather_map(dir_data, map):
    zip_file = os.path.join(dir_data, map + ".7z")
    if not os.path.exists(zip_file):
        print "Downloading map file " + map + ".7z ..."
        socket.setdefaulttimeout(10)
        urllib.urlretrieve(gather_from_server + map + ".7z", zip_file)

    if not os.path.exists(zip_file):
        return False
     
    print "Decompressing map file " + map + ".7z ..."
    arg = [cmd_7zip, "x", "-o" + dir_data, zip_file]
    p = subprocess.Popen(arg)
    p.wait()
    
    os.unlink(zip_file)

    return True
    

def __check_map(dir_data, map):
    return os.path.exists(os.path.join(dir_data, map))
    
def __create_layer_from_map(bounds, layer, map, overwrite, dir_data, dir_temp):
    if not isinstance(bounds, GeoRect):
        return False
    
    if not __check_map(dir_data, map):
        if not __gather_map(dir_data, map):
            return False

    arg = [cmd_ogr2ogr]

    if overwrite == True:
        arg.append("-overwrite")
    else:
        arg.append("-update")
        arg.append("-append")

    if layer[1] != "":
        arg.extend(["-where", layer[2]])

    arg.extend(["-select", layer[3]])

    arg.extend(["-spat",
                str(bounds.left.value_degrees()),
                str(bounds.bottom.value_degrees()),
                str(bounds.right.value_degrees()),
                str(bounds.top.value_degrees())])

    arg.append(dir_temp)
    arg.append(os.path.join(dir_data, map))

    arg.append(layer[0])
    arg.extend(["-nln", layer[1]])

    p = subprocess.Popen(arg)
    p.wait()

    return True

def __create_layer(bounds, layer, dir_data, dir_temp):
    print "Creating topology layer " + layer[1] + " ..."

    for i in range(len(__maps)):
        __create_layer_from_map(bounds, layer, __maps[i],
                                i == 0, dir_data, dir_temp)

    files = []
    if os.path.exists(os.path.join(dir_temp, layer[1] + ".shp")):
        files.append([os.path.join(dir_temp, layer[1] + ".shp"), True])
        files.append([os.path.join(dir_temp, layer[1] + ".shx"), True])
        files.append([os.path.join(dir_temp, layer[1] + ".dbf"), True])
        files.append([os.path.join(dir_temp, layer[1] + ".prj"), True])

    return files

def __create_layers(bounds, dir_data, dir_temp):
    files = []
    for layer in __layers:
        files.extend(__create_layer(bounds, layer, dir_data, dir_temp))

    return files

def __create_index_file(dir_temp):
    file = open(os.path.join(dir_temp, "topology.tpl"), "w")
    file.write("* filename,range,icon,field\n");
    file.write("inwaterahydro_area, 100,,,64,96,240\n");
    file.write("watrcrslhydro_line,    7,,,64,96,240\n");
    file.write("builtupapop_area,  15,,1,223,223,0\n");
    file.write("roadltrans_line,     15,,,240,64,64\n");
    file.write("railrdltrans_line,   10,,,64,64,64\n");
    file.write("mispopppop_point,    5,218,1,223,223,0\n");
    file.close()
    return os.path.join(dir_temp, "topology.tpl")

def Create(bounds, dir_data = "../data/", dir_temp = "../tmp/"):
    dir_data = os.path.abspath(os.path.join(dir_data, "vmap0"))
    dir_temp = os.path.abspath(dir_temp)

    files = __create_layers(bounds, dir_data, dir_temp)
    files.append([__create_index_file(dir_temp), True])

    return files
