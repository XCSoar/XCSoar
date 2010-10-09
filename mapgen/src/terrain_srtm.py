import os
import math
import subprocess
import urllib
import socket

from georect import GeoRect
from zipfile import ZipFile, BadZipfile

cmd_gdal_warp = "gdalwarp"
cmd_geojasper = "geojasper"
#gather_from_server = None
#gather_from_server = "ftp://srtm.csi.cgiar.org/SRTM_V41/SRTM_Data_GeoTiff/"
#gather_from_server = "http://srtm.csi.cgiar.org/SRT-ZIP/SRTM_V41/SRTM_Data_GeoTiff/"
#gather_from_server = "http://droppr.org/srtm/v4.1/6_5x5_TIFs/"
gather_from_server = "ftp://xftp.jrc.it/pub/srtmV4/tiff/"
#gather_from_server = "http://webbuster.dyndns.info/xcsoar/mapgen_data/terrain/srtm/"

use_world_file = True

'''
 1) Gather tiles
'''
def __get_tile_name(lat, lon):
    col = ((lon + 180) / 5) + 1
    row = (60 - lat) / 5
    if col < 1 or col > 72 or row < 1 or row > 24:
        return None

    return "srtm_%02d_%02d" % (col, row)

def __download_tile(path_tile_zip, filename):
    socket.setdefaulttimeout(10)
    try:
        urllib.urlretrieve(gather_from_server + filename + ".zip", 
                           path_tile_zip)
    except IOError:
        print "Download of tile " + filename + " failed!"

def __extract_tile(path_tile_zip, dir_temp, filename):
    try:
        zip = ZipFile(path_tile_zip, "r")
    except BadZipfile:
        print "Decompression of the file "+filename+".zip failed!"
        return False

    zip.extract(filename + ".tif", dir_temp)
    zip.close()
    return True
    
def __gather_tile(dir_data, dir_temp, lat, lon):
    # generate filename to search for
    filename = __get_tile_name(lat, lon)
    if filename == None:
        return None

    # check if the GeoTIFF file already exists in the temporary folder
    path_tile = os.path.join(dir_temp, filename + ".tif")
    if os.path.exists(path_tile):
        print "Tile " + filename + " found!"
        return path_tile

    # check if the GeoTIFF file exists in the data folder
    path_tile = os.path.join(dir_data, filename + ".tif")
    if os.path.exists(path_tile):
        print "Tile " + filename + " found!"
        return path_tile

    path_tile_zip = os.path.join(dir_data, filename + ".zip")
    for i in range(1, 4):
        # if the ZIP file doesn't exist in the data folder try to download it
        if gather_from_server != None:
            if not os.path.exists(path_tile_zip):
                print "Downloading tile " + filename + " from the internet ..."
                __download_tile(path_tile_zip, filename)
    
        # check if the ZIP file exists in the data folder
        if not os.path.exists(path_tile_zip):
            print "Tile " + filename + " can not be found!"
            return None
    
        print "Tile " + filename + " found inside zip file! -> Decompressing ..."
        if not __extract_tile(path_tile_zip, dir_temp, filename):
            os.unlink(path_tile_zip)
            continue
    
        # check if the GeoTIFF file now exists in the temporary folder
        path_tile = os.path.join(dir_temp, filename + ".tif")
        if os.path.exists(path_tile):
            return path_tile

    print "Decompression failed!"
    return None

def __gather_tiles(dir_data, dir_temp, bounds):
    '''
    Makes sure the terrain tiles are available at a certain location.
    @param dir_data: Data path
    @param dir_temp: Temporary path
    @param bounds: Bounding box (GeoRect)
    @return: The list of tile files
    '''
    if not isinstance(bounds, GeoRect):
        return None

    print "Gathering terrain tiles ..."

    # Calculate rounded bounds
    lat_start = int(math.floor(bounds.bottom.value_degrees() / 5.0)) * 5
    lon_start = int(math.floor(bounds.left.value_degrees() / 5.0)) * 5
    lat_end = int(math.ceil(bounds.top.value_degrees() / 5.0)) * 5
    lon_end = int(math.ceil(bounds.right.value_degrees() / 5.0)) * 5

    tiles = []
    # Iterate through latitude and longitude in 5 degree interval
    for lat in range(lat_start, lat_end, 5):
        for lon in range(lon_start, lon_end, 5):
            tile = __gather_tile(dir_data, dir_temp, lat, lon)
            if tile != None:
                # If tile is available append its filename to the tiles list
                tiles.append(tile)

    # Return list of available tile files
    return tiles

'''
 2) Merge tiles into big tif, Resample and Crop merged image
    gdalwarp
    -r cubic
        (Resampling method to use. Cubic resampling.)
    -tr $degrees_per_pixel $degrees_per_pixel
        (set output file resolution (in target georeferenced units))
    -wt Int16
        (Working pixel data type. The data type of pixels in the source
         image and destination image buffers.)
    -dstnodata -31744
        (Set nodata values for output bands (different values can be supplied
         for each band). If more than one value is supplied all values should
         be quoted to keep them together as a single operating system argument.
         New files will be initialized to this value and if possible the
         nodata value will be recorded in the output file.)
    -te $left $bottom $right $top
        (set georeferenced extents of output file to be created (in target SRS))
    a.tif b.tif c.tif ...
        (Input files)
    terrain.tif
        (Output file)
'''
def __create(dir_temp, tiles, arcseconds_per_pixel, bounds):
    print "Resampling terrain ..."
    output_file = os.path.join(dir_temp, "terrain.tif")
    if os.path.exists(output_file):
        os.unlink(output_file)

    degree_per_pixel = float(arcseconds_per_pixel) / 3600.0

    args = [cmd_gdal_warp,
            "-r", "cubic",
            "-tr", str(degree_per_pixel), str(degree_per_pixel),
            "-wt", "Int16",
            "-dstnodata", "-31744",
            "-multi"]

    if use_world_file == True:
        args.extend(["-co", "TFW=YES"])
    
    args.extend(["-te", str(bounds.left.value_degrees()),
                str(bounds.bottom.value_degrees()),
                str(bounds.right.value_degrees()),
                str(bounds.top.value_degrees())])

    args.extend(tiles)
    args.append(output_file)

    try:
        p = subprocess.Popen(args)
    except OSError, WindowsError:
        print ("There has been a problem running the gdalwarp application "+
               "that creates the appropriate GeoTIFF terrain file!")
        print ("Please check the \"cmd_gdal_warp\" variable in the "+
               "terrain_srtm module and modify it if necessary.")
        print "Current value: \""+cmd_gdal_warp+"\""
        return None
    
    p.wait()

    return output_file

'''
 3) Convert to GeoJP2 with GeoJasPer
    cmd_geojasper
    -f blabla_cropped.tif
        (Input file name)
    -F terrain.jp2
        (Output file name)
    -T jp2
        (Output type)
    -O rate=0.1
        (Compression rate, 10 times)
    -O tilewidth=256
        (generate tiled image using tile width 256)
    -O tileheight=256
        (generate tiled image using tile height 256)
    -O xcsoar=1
        (???)
'''
def __convert(dir_temp, input_file, rc):
    print "Converting terrain to JP2 format ..."
    output_file = os.path.join(dir_temp, "terrain.jp2")
    if os.path.exists(output_file):
        os.unlink(output_file)

    args = [cmd_geojasper,
            "-f", input_file,
            "-F", output_file,
            "-T", "jp2",
            "-O", "rate=0.1",
            "-O", "tilewidth=256",
            "-O", "tileheight=256"]
    
    if not use_world_file:
        args.extend(["-O", "xcsoar=1",
                     "-O", "lonmin=" + str(rc.left.value_degrees()),
                     "-O", "lonmax=" + str(rc.right.value_degrees()),
                     "-O", "latmax=" + str(rc.top.value_degrees()),
                     "-O", "latmin=" + str(rc.bottom.value_degrees())])

    try:
        p = subprocess.Popen(args)
    except OSError, WindowsError:
        print ("There has been a problem running the geojasper application "+
               "that converts the GeoTIFF terrain file to GeoJP2!")
        print ("Please check the \"cmd_geojasper\" variable in the "+
               "terrain_srtm module and modify it if necessary.")
        print "Current value: \""+cmd_geojasper+"\""
        return None
    
    p.wait()
    
    output = [[output_file, False]]
    
    world_file_tiff = os.path.join(dir_temp, "terrain.tfw")
    world_file = os.path.join(dir_temp, "terrain.j2w")
    if use_world_file and os.path.exists(world_file_tiff):
        os.rename(world_file_tiff, world_file)
        output.append([world_file, True])

    return output

def __cleanup(dir_temp):
    for file in os.listdir(dir_temp):
        if  file.endswith(".tif") and (file.startswith("srtm_") or
                                       file.startswith("terrain")):
            os.unlink(os.path.join(dir_temp, file))

def Create(bounds, arcseconds_per_pixel = 9.0,
           dir_data = "../data/", dir_temp = "../tmp/"):
    dir_data = os.path.abspath(os.path.join(dir_data, "srtm"))
    dir_temp = os.path.abspath(dir_temp)

    if not os.access(dir_data, os.X_OK):
        os.makedirs(dir_data)

    # Make sure the tiles are available
    tiles = __gather_tiles(dir_data, dir_temp, bounds)
    if len(tiles) < 1:
        return None

    terrain_file = __create(dir_temp, tiles, arcseconds_per_pixel, bounds)
    if terrain_file == None:
        return None
    
    final_files = __convert(dir_temp, terrain_file, bounds)
    if final_files == None:
        return None
    
    __cleanup(dir_temp)

    return final_files
