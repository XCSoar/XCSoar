import urllib
import os
import subprocess
import socket

from xcsoar.mapgen.georect import GeoRect
from xcsoar.mapgen.filelist import FileList

__cmd_ogr2ogr = "ogr2ogr"
__cmd_7zip = "7za"
__cmd_shptree = "shptree"
__server_base_path = "http://download.xcsoar.org/mapgen/data/"

__vmap0_datasets = [
    { 'name': 'vmap0/eur', 'bounds': GeoRect( -35, 180, 90,  30) },
    { 'name': 'vmap0/noa', 'bounds': GeoRect(-180,   0, 90, -40) },
    { 'name': 'vmap0/sas', 'bounds': GeoRect(  25, 180, 50, -60) },
    { 'name': 'vmap0/soa', 'bounds': GeoRect(-180, 180, 30, -90) },
    ]

__osm_datasets = [
    { 'name': 'osm/planet', 'bounds': GeoRect(-180, 180, 90, -90) },
    ]

__layers = [
    { 'name': 'citybig_point',   'datasets': __osm_datasets,   'layer': 'citybig_point',        'label': 'name', 'where': '',       'range':  10, 'color': '223,223,0'  },
    { 'name': 'citysmall_point', 'datasets': __osm_datasets,   'layer': 'citysmall_point',      'label': 'name', 'where': '',       'range':   5, 'color': '223,223,0'  },
    { 'name': 'roadbig_line',    'datasets': __osm_datasets,   'layer': 'roadbig_line',         'label': '',     'where': '',       'range':  15, 'color': '240,64,64'  },
    { 'name': 'roadmedium_line', 'datasets': __osm_datasets,   'layer': 'roadmedium_line',      'label': '',     'where': '',       'range':   8, 'color': '240,64,64'  },
    { 'name': 'roadsmall_line',  'datasets': __osm_datasets,   'layer': 'roadsmall_line',       'label': '',     'where': '',       'range':   3, 'color': '240,64,64'  },
    { 'name': 'city_area',       'datasets': __vmap0_datasets, 'layer': 'pop-built-up-a',       'label': 'nam',  'where': '',       'range':  50, 'color': '223,223,0'  },
    { 'name': 'water_line',      'datasets': __vmap0_datasets, 'layer': 'hydro-water-course-l', 'label': 'nam',  'where': 'hyc=8',  'range':   7, 'color': '85,160,255' },
    { 'name': 'water_area',      'datasets': __vmap0_datasets, 'layer': 'hydro-inland-water-a', 'label': 'nam',  'where': 'hyc=8',  'range': 100, 'color': '85,160,255' },
    { 'name': 'railroad_line',   'datasets': __vmap0_datasets, 'layer': 'trans-railroad-l'    , 'label': 'fco',  'where': 'exs=28', 'range':  10, 'color': '64,64,64'   },
    ]

def __filter_datasets(bounds, datasets):
     filtered = []
     for dataset in datasets:
         if bounds.intersects(dataset['bounds']):
             filtered.append(dataset)
     return filtered

def __gather_dataset(dir_data, dataset):
    file_name = dataset['name'] + '.7z'
    zip_file = os.path.join(dir_data, file_name)
    if not os.path.exists(zip_file):
        print "Downloading topology dataset " + __server_base_path + file_name + " ..."
        if not os.path.exists(os.path.dirname(zip_file)):
             os.makedirs(os.path.dirname(zip_file))
        socket.setdefaulttimeout(10)
        urllib.urlretrieve(__server_base_path + file_name, zip_file)

    if not os.path.exists(zip_file):
        raise RuntimeError, "Failed to download " + file_name

    print "Decompressing map file " + file_name + " ..."
    arg = [__cmd_7zip, 'x', '-y', '-o' + os.path.dirname(zip_file), zip_file]

    try:
        p = subprocess.Popen(arg)
        p.wait()
    except Exception, e:
        print "Executing " + str(arg) + " failed"
        raise

    os.unlink(zip_file)

def __create_layer_from_dataset(bounds, layer, dataset, overwrite, dir_data, dir_temp):
    if not isinstance(bounds, GeoRect):
        raise TypeError

    if not os.path.exists(os.path.join(dir_data, dataset['name'])):
        __gather_dataset(dir_data, dataset)

    print "Reading dataset " + dataset['name']  + " ..."
    arg = [__cmd_ogr2ogr]

    if overwrite == True:
        arg.append("-overwrite")
    else:
        arg.append("-update")
        arg.append("-append")

    if layer['where'] != '':
        arg.extend(["-where", layer['where']])

    arg.extend(["-select", layer['label']])

    arg.extend(["-spat",
                str(bounds.left),
                str(bounds.bottom),
                str(bounds.right),
                str(bounds.top)])

    arg.append(dir_temp)
    arg.append(os.path.join(dir_data, dataset['name']))

    arg.append(layer['layer'])
    arg.extend(['-nln', layer['name']])

    try:
        p = subprocess.Popen(arg)
        p.wait()
    except Exception, e:
        print "Executing " + str(arg) + " failed"
        raise

def __create_layer_index(layer, dir_temp):
    print "Generating index file for layer " + layer['name'] + " ..."
    arg = [__cmd_shptree]

    arg.append(os.path.join(dir_temp, layer['name'] + ".shp"))

    try:
        p = subprocess.Popen(arg)
        p.wait()
    except Exception, e:
        print "Executing " + str(arg) + " failed"
        raise

def __create_layer(bounds, layer, dir_data, dir_temp, files, index):
    print "Creating topology layer " + layer['name'] + " ..."

    datasets = __filter_datasets(bounds, layer['datasets'])
    for i in range(len(datasets)):
        __create_layer_from_dataset(bounds, layer, datasets[i],
                                    i == 0, dir_data, dir_temp)

    if os.path.exists(os.path.join(dir_temp, layer['name'] + '.shp')):
        __create_layer_index(layer, dir_temp)

        files.add(os.path.join(dir_temp, layer['name'] + '.shp'), False)
        files.add(os.path.join(dir_temp, layer['name'] + '.shx'), False)
        files.add(os.path.join(dir_temp, layer['name'] + '.dbf'), False)
        files.add(os.path.join(dir_temp, layer['name'] + '.prj'), False)
        files.add(os.path.join(dir_temp, layer['name'] + '.qix'), False)
        index.append({ 'name': layer['name'], 'range': layer['range'], 'color': layer['color'], 'label': layer['label'] })

def __create_index_file(dir_temp, index):
    file = open(os.path.join(dir_temp, 'topology.tpl'), 'w')
    file.write("* filename, range, icon, label_index, r, g, b\n")
    for layer in index:
        file.write(layer['name'] + ',' + str(layer['range']) + ',,' +
                   ('' if layer['label'] == '' else '1') + ',' + layer['color'] + "\n")
    file.close()
    return os.path.join(dir_temp, "topology.tpl")

def create(bounds, dir_data, dir_temp):
    files = FileList()
    index = []
    for layer in __layers:
        __create_layer(bounds, layer, dir_data, dir_temp, files, index)

    files.add(__create_index_file(dir_temp, index), False)
    return files
