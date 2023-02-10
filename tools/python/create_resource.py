import sys
import os
import io

debug = False

print('=========  CREATE_RESOURCE ===============')
print('CurrDir: ', os.getcwd())

res_array = []


# def write_resource(outfile, line, macro, type, extension):
def write_resource(outfile, line, resource):
         line = line.replace(resource[0]+'(','')
         line = line.replace(')','')
         params = line.split(',')
         # if debug:
         if len(params) == 2:
             line = '{:30s}'.format(params[0]) + ' ' + '{:12s}'.format(resource[1])
             line = line + 'DISCARDABLE   L"' + resource[2] + '/'
             line = line + params[1].strip(' \n').replace('"','') + resource[3] + '"'
             # if len(line) > 0:
             # outfile.write(line.replace('/', '\\\\') +  '\n') # Copy of read line (with replaced field)
             outfile.write(line + '\n') # Copy of read line (with replaced field)
             if debug:
                print(line)

def write_line(outfile, line):
      line = line.strip()
        
      if line.startswith('#include') or \
           line.startswith('ID') or \
           line.startswith('#if') or \
           line.startswith('#else') or \
           line.startswith('#elif') or \
           line.startswith('#endif'):
           outfile.write(line+ '\n') # Copy of read line (with replaced field)
           print(line + '\n')
      else:
        updated = False
        for resource in res_array:
           if line.startswith(resource[0]):
              write_resource(outfile, line, resource) # 'BITMAP_ICON', 'ICON', '.bmp')
              updated = True  # if one of this lines

        if not updated: 
           outfile.write('\n') # Copy of read line (with replaced field)
           # outfile.write(line+ '\n') # Copy of read line (with replaced field)
           # print(line + '\n')
#============================================================================

if debug:
  count = 0
  for arg in sys.argv:
      print('argument ',count + 1,': ', sys.argv[count])
      count = count + 1

src_location1 = sys.argv[3]
src_location2 = sys.argv[4]

res_array.append(['BITMAP_ICON',   'BITMAP', src_location2 + '/icons', '.bmp'])
res_array.append(['BITMAP_BITMAP', 'BITMAP', src_location1 + '/bitmaps', '.bmp'])
res_array.append(['BITMAP_GRAPHIC','BITMAP', src_location2 + '/graphics', '.bmp'])
res_array.append(['HATCH_BITMAP',  'BITMAP', src_location1 + '/bitmaps', '.bmp'])
res_array.append(['SOUND',         'WAVE',   src_location1 + '/sound', '.wav'])
res_array.append(['ICON_ICON',     'ICON',   src_location1 + '/bitmaps', '.ico'])

if len(sys.argv) < 2:
   print('to less arguments: ', len(sys.argv))
else:
    infile = io.open(sys.argv[1])
    outfile = io.open(sys.argv[2], 'w', newline='\n')
    
    for line in infile:
       write_line(outfile, line)
    infile.close()
    outfile.close()
    