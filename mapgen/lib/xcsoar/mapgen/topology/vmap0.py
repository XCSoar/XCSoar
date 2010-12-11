import urllib
import os
import subprocess
import socket

from xcsoar.mapgen.georect import GeoRect
from xcsoar.mapgen.filelist import FileList

cmd_ogr2ogr = "ogr2ogr"
cmd_7zip = "7za"
cmd_shptree = "shptree"

gather_from_server = "http://download.xcsoar.org/mapgen/data/vmap0/"

__maps = [["eur", GeoRect(-35, 180, 90, 30)],
          ["noa", GeoRect(-180, 0, 90, -40)],
          ["sas", GeoRect(25, 180, 50, -60)],
          ["soa", GeoRect(-180, 180, 30, -90)]]
__layers = [["pop-miscellaneous-population-p", "mispopppop_point", "", "txt"],
            ["pop-built-up-a", "builtupapop_area", "", "nam"],
            ["hydro-water-course-l", "watrcrslhydro_line", "hyc=8", "nam"],
            ["hydro-inland-water-a", "inwaterahydro_area", "hyc=8", "nam"],
            ["trans-railroad-l", "railrdltrans_line", "exs=28", "fco"],
            ["trans-road-l", "roadltrans_line", "rtt=14", "med"]]

def __filter_maps(bounds, maps = __maps):
    maps_filtered = []
    for map in maps:
        if bounds.intersects(map[1]):
            maps_filtered.append(map)

    return maps_filtered

def __gather_map(dir_data, map_name):
    zip_file = os.path.join(dir_data, map_name + ".7z")
    if not os.path.exists(zip_file):
        print "Downloading map file " + map_name + ".7z ..."
        socket.setdefaulttimeout(10)
        urllib.urlretrieve(gather_from_server + map_name + ".7z", zip_file)

    if not os.path.exists(zip_file):
        raise RuntimeError, "Failed to download " + map_name

    print "Decompressing map file " + map_name + ".7z ..."
    arg = [cmd_7zip, "x", "-o" + dir_data, zip_file]

    try:
        p = subprocess.Popen(arg)
        p.wait()
    except Exception, e:
        print "Executing " + str(arg) + " failed"
        raise

    os.unlink(zip_file)

def __create_layer_from_map(bounds, layer, map_name, overwrite, dir_data, dir_temp):
    if not isinstance(bounds, GeoRect):
        raise TypeError

    if not os.path.exists(os.path.join(dir_data, map_name)):
        __gather_map(dir_data, map_name)

    print "Reading map " + map_name + " ..."
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
    arg.append(os.path.join(dir_data, map_name))

    arg.append(layer[0])
    arg.extend(["-nln", layer[1]])

    try:
        p = subprocess.Popen(arg)
        p.wait()
    except Exception, e:
        print "Executing " + str(arg) + " failed"
        raise

def __create_layer_index(layer, dir_temp):
    print "Generating index file for layer " + layer[1] + " ..."
    arg = [cmd_shptree]

    arg.append(os.path.join(dir_temp, layer[1] + ".shp"))

    try:
        p = subprocess.Popen(arg)
        p.wait()
    except Exception, e:
        print "Executing " + str(arg) + " failed"
        raise

def __create_layer(bounds, layer, maps, dir_data, dir_temp):
    print "Creating topology layer " + layer[1] + " ..."

    for i in range(len(maps)):
        __create_layer_from_map(bounds, layer, maps[i][0],
                                i == 0, dir_data, dir_temp)

    files = FileList()
    if os.path.exists(os.path.join(dir_temp, layer[1] + ".shp")):
        __create_layer_index(layer, dir_temp)

        files.add(os.path.join(dir_temp, layer[1] + ".shp"), False)
        files.add(os.path.join(dir_temp, layer[1] + ".shx"), False)
        files.add(os.path.join(dir_temp, layer[1] + ".dbf"), False)
        files.add(os.path.join(dir_temp, layer[1] + ".prj"), False)

    if os.path.exists(os.path.join(dir_temp, layer[1] + ".qix")):
        files.add(os.path.join(dir_temp, layer[1] + ".qix"), False)

    return files

def __create_layers(bounds, maps, dir_data, dir_temp):
    files = FileList()
    for layer in __layers:
        files.extend(__create_layer(bounds, layer, maps, dir_data, dir_temp))

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

def create(bounds, dir_data = "../data/", dir_temp = "../tmp/"):
    dir_data = os.path.abspath(os.path.join(dir_data, "vmap0"))
    dir_temp = os.path.abspath(dir_temp)

    if not os.access(dir_data, os.X_OK):
        os.makedirs(dir_data)

    maps = __filter_maps(bounds)
    files = __create_layers(bounds, maps, dir_data, dir_temp)
    files.add(__create_index_file(dir_temp), False)

    return files
