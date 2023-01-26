import sys

print('argument 1: ', sys.argv[0])
if len(sys.argv) < 3:
   print('to less arguments: ', len(sys.argv))
else:
    fname = sys.argv[1]
    infile = open(fname)
    outfile = open(sys.argv[2], 'w')

    
    repl = []  
    count = 0
    for arg in sys.argv[3:]:
           # print(count, ': ', arg)
           repl.append(arg.split(';'))
           print(count, ': ', repl[count][0], ' ==> ', repl[count][1])
           count = count + 1
    # zeilenweise Iteration schont den Speicher
    
    for line in infile:
      for i in [0, count-1]:
          line = line.replace(repl[i][0],repl[i][1])
      outfile.write(line) # kopie der eingelesenen Zeile

    infile.close()
    outfile.close()