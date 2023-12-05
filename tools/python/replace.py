import sys

debug = False
headerfile = None

if debug:
  count = 0
  for arg in sys.argv:
      print('argument ',count + 1,': ', sys.argv[count])
      count = count + 1

if len(sys.argv) < 3:
   print('to less arguments: ', len(sys.argv))
else:
    infile = open(sys.argv[2])
    outfile = open(sys.argv[3], 'w')
    try:
        headerfile = open(sys.argv[4], 'w')
    except Exception as e:
        print("Exception on creating '", sys.argv[4], "'")
        print("Exception: ", e)
        # no headerfile ? exit(-1)

    repl = []  
    count = 0
    configfile = open(sys.argv[1])
    for line in configfile:
          if debug:
            print('argument ',count + 1,': ', line)
          config_pair = line.split('=')
          if len(config_pair) == 2:
            repl.append([config_pair[0].strip(), config_pair[1].strip()])
          count = count + 1
    configfile.close()
    
    for param in repl:
      if len(param) == 2:
        if debug:
           print('param0 = ',param[0],', param1 = ',param[1])
        if headerfile:
           headerfile.write('#define ' + '{:24s}'.format(param[0]) + ' "' + param[1] + '"\n')
    
    for line in infile:
      for param in repl:
        if len(param) == 2:
         line = line.replace('@'+param[0]+'@',param[1])
      outfile.write(line) # Copy of read line (with replaced field)
    
    infile.close()
    outfile.close()
    if headerfile:
        headerfile.close()

print('EOF replace.py')
exit(0)