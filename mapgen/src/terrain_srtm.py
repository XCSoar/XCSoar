import os
import math
import subprocess
import urllib
import socket

from georect import GeoRect
from zipfile import ZipFile

cmd_gdal_merge = "gdal_merge.py"
cmd_gdal_warp = "gdalwarp"
cmd_geojasper = "geojasper"
#gather_from_server = None
#gather_from_server = "ftp://srtm.csi.cgiar.org/SRTM_V41/SRTM_Data_GeoTiff/"
#gather_from_server = "http://srtm.csi.cgiar.org/SRT-ZIP/SRTM_V41/SRTM_Data_GeoTiff/"
#gather_from_server = "http://droppr.org/srtm/v4.1/6_5x5_TIFs/"
gather_from_server = "ftp://xftp.jrc.it/pub/srtmV4/tiff/"
#gather_from_server = "http://webbuster.dyndns.info/xcsoar/mapgen_data/terrain/srtm/"

'''
 1) Gather tiles
'''
def __get_tile_name(lat, lon):
    col = ((lon + 180) / 5) + 1
    row = (60 - lat) / 5
    if col < 1 or col > 72 or row < 1 or row > 24:
        return None

    return "srtm_%02d_%02d" % (col, row)

def __gather_tile(dir_data, dir_temp, lat, lon):
    # generate filename to search for
    filename = __get_tile_name(lat, lon)
    if filename == None:
        return None

    # check if the GeoTIFF file already exists in the temporary folder
    if os.path.exists(os.path.join(dir_temp, filename + ".tif")):
        print "Tile " + filename + " found!"
        return os.path.join(dir_temp, filename + ".tif")

    # check if the GeoTIFF file exists in the data folder
    if os.path.exists(os.path.join(dir_data, filename + ".tif")):
        print "Tile " + filename + " found!"
        return os.path.join(dir_data, filename + ".tif")

    # if the ZIP file doesn't exist in the data folder try to download it
    if gather_from_server != None:
        if not os.path.exists(os.path.join(dir_data, filename + ".zip")):
            print "Downloading tile " + filename + " from the internet ..."
            socket.setdefaulttimeout(10)
            urllib.urlretrieve(gather_from_server + filename + ".zip",
                               os.path.join(dir_data, filename + ".zip"))

    # check if the ZIP file exists in the data folder
    if not os.path.exists(os.path.join(dir_data, filename + ".zip")):
        print "Tile " + filename + " can not be found!"
        return None

    print "Tile " + filename + " found inside zip file! -> Decompressing ..."
    # decompress the ZIP file to the temporary folder
    zip = ZipFile(os.path.join(dir_data, filename + ".zip"), "r")
    zip.extract(filename + ".tif", dir_temp)
    zip.close()

    # check if the GeoTIFF file now exists in the temporary folder
    if os.path.exists(os.path.join(dir_temp, filename + ".tif")):
        return os.path.join(dir_temp, filename + ".tif")

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
 2) Merge tiles into big tif
    cmd_gdal_merge.py
    -n -32768
        (Ignore pixels from files being merged in with this pixel value.)
    -init -32768
        (Pre-initialize the output image bands with these values. However,
         it is not marked as the nodata value in the output file. If only one
         value is given, the same value is used in all the bands.)
    -o blabla_merged.tif
        (The name of the output file, which will be created if it does
         not already exist.)
    a.tif b.tif c.tif ...
        (Input files)
'''
def __merge_tiles(dir_temp, tiles):
    print "Merging terrain tiles ..."
    output_file = os.path.join(dir_temp, "terrain_merged.tif")
    if os.path.exists(output_file):
        os.unlink(output_file)

    args = [cmd_gdal_merge,
            "-n", "-32768",
            "-init", "-32768",
            "-o", output_file]
    args.extend(tiles)

    p = subprocess.Popen(args)
    p.wait()

    return output_file

'''
 3) Resample merged image
    gdalwarp
    -r cubicspline
        (Resampling method to use. Cubic spline resampling.)
    -tr $degrees_per_pixel $degrees_per_pixel
        (set output file resolution (in target georeferenced units))
    -wo "INTERLEAVE=BIL"
        (Set a warp options. The GDALWarpOptions::papszWarpOptions docs show
         all options. Multiple -wo options may be listed.)
    -wt Int16
        (Working pixel data type. The data type of pixels in the source
         image and destination image buffers.)
    -of "GTiff"
        (Select the output format. The default is GeoTIFF (GTiff).
         Use the short format name.)
    -srcnodata -32768
        (Set nodata masking values for input bands (different values can be
         supplied for each band). If more than one value is supplied all values
         should be quoted to keep them together as a single operating system
         argument. Masked values will not be used in interpolation. Use a value
         of None to ignore intrinsic nodata settings on the source dataset.)
    -dstnodata -1
        (Set nodata values for output bands (different values can be supplied
         for each band). If more than one value is supplied all values should
         be quoted to keep them together as a single operating system argument.
         New files will be initialized to this value and if possible the
         nodata value will be recorded in the output file.)
    blabla_merged.tif
        (Input file)
    blabla_resampled.tif
        (Output file)
'''
def __resample(dir_temp, input_file, arcseconds_per_pixel):
    print "Resampling terrain ..."
    output_file = os.path.join(dir_temp, "terrain_resampled.tif")
    if os.path.exists(output_file):
        os.unlink(output_file)

    degree_per_pixel = float(arcseconds_per_pixel) / 3600.0

    args = [cmd_gdal_warp,
            "-r", "cubicspline",
            "-tr", str(degree_per_pixel), str(degree_per_pixel),
            "-wo", "INTERLEAVE=BIL",
            "-wt", "Int16",
            "-of", "GTiff",
            "-srcnodata", "-32768",
            "-dstnodata", "-1",
            input_file,
            output_file]

    p = subprocess.Popen(args)
    p.wait()

    return output_file

'''
 4) Crop resampled image
    gdalwarp
    -srcnodata -1
        (Set nodata masking values for input bands (different values can be
         supplied for each band). If more than one value is supplied all values
         should be quoted to keep them together as a single operating system
         argument. Masked values will not be used in interpolation. Use a value
         of None to ignore intrinsic nodata settings on the source dataset.)
    -dstnodata -1
        (Set nodata values for output bands (different values can be supplied
         for each band). If more than one value is supplied all values should
         be quoted to keep them together as a single operating system argument.
         New files will be initialized to this value and if possible the
         nodata value will be recorded in the output file.)
    -te $left $bottom $right $top
        (set georeferenced extents of output file to be created (in target SRS))
    blabla_resampled.tif
        (Input file)
    blabla_cropped.tif
        (Output file)
'''
def __crop(dir_temp, input_file, rc):
    print "Cropping terrain ..."
    output_file = os.path.join(dir_temp, "terrain.tif")
    if os.path.exists(output_file):
        os.unlink(output_file)

    args = [cmd_gdal_warp,
            "-srcnodata", "-1",
            "-dstnodata", "-1",
            "-te", str(rc.left.value_degrees()),
            str(rc.bottom.value_degrees()),
            str(rc.right.value_degrees()),
            str(rc.top.value_degrees()),
            input_file,
            output_file]

    p = subprocess.Popen(args)
    p.wait()

    return output_file

'''
 5) Convert to GeoJP2 with GeoJasPer
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
    print "Converting terrain to GeoJP2 format ..."
    output_file = os.path.join(dir_temp, "terrain.jp2")
    if os.path.exists(output_file):
        os.unlink(output_file)

    args = [cmd_geojasper,
            "-f", input_file,
            "-F", output_file,
            "-T", "jp2",
            "-O", "rate=0.1",
            "-O", "tilewidth=256",
            "-O", "tileheight=256",
            "-O", "xcsoar=1",
            "-O", "lonmin=" + str(rc.left.value_degrees()),
            "-O", "lonmax=" + str(rc.right.value_degrees()),
            "-O", "latmax=" + str(rc.top.value_degrees()),
            "-O", "latmin=" + str(rc.bottom.value_degrees())]

    p = subprocess.Popen(args)
    p.wait()

    return output_file

def __cleanup(dir_temp):
    for file in os.listdir(dir_temp):
        if  file.endswith(".tif") and (file.startswith("srtm_") or
                                       file.startswith("terrain")):
            os.unlink(os.path.join(dir_temp, file))

def Create(bounds, arcseconds_per_pixel = 9.0,
           dir_data = "../data/", dir_temp = "../tmp/"):
    dir_data = os.path.abspath(os.path.join(dir_data, "srtm"))
    dir_temp = os.path.abspath(dir_temp)

    # Make sure the tiles are available
    tiles = __gather_tiles(dir_data, dir_temp, bounds)
    if len(tiles) < 1:
        return None

    merged_file = __merge_tiles(dir_temp, tiles)
    resampled_file = __resample(dir_temp, merged_file, arcseconds_per_pixel)
    cropped_file = __crop(dir_temp, resampled_file, bounds)
    converted_file = __convert(dir_temp, cropped_file, bounds)
    __cleanup(dir_temp)

    return [converted_file, False]

