import sys

debug = True

if debug:
  count = 0
  for arg in sys.argv:
      print('argument ',count + 1,': ', sys.argv[count])
      count = count + 1

if len(sys.argv) < 3:
   print('to less arguments: ', len(sys.argv))
else:
    fname = sys.argv[1]
    infile = open(fname)
    outfile = open(sys.argv[2], 'w')

    repl = []  
    count = 0
    configfile = open(sys.argv[3])
    for line in configfile:
          if debug:
            print('argument ',count + 1,': ', line)
          config_pair = line.split('=')
          if len(config_pair) == 2:
            repl.append([config_pair[0].strip(), config_pair[1].strip()])
          count = count + 1
    
    if debug:
      for param in repl:
        if len(param) == 2:
           print('param0 = ',param[0],', param1 = ',param[1])
    
    for line in infile:
      for param in repl:
        if len(param) == 2:
         line = line.replace('@'+param[0]+'@',param[1])
      outfile.write(line) # Copy of read line (with replaced field)
    
    infile.close()
    outfile.close()