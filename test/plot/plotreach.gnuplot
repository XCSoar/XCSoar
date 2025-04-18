#set terminal png size 1024,1024 crop
#set output '| display png:-'

set size ratio -1
set key outside right

set style line 1 lt 2 lc rgb "black" lw 4
set style line 2 lt 4 lc rgb "purple" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1
set style line 4 lt 4 lc rgb "blue" lw 2
set style line 5 lt 4 lc rgb "grey" lw 2
set style line 6 lt 4 lc rgb "green" lw 1
set style line 7 lt 4 lc rgb "cyan" lw 1
set style line 8 lt 2 lc rgb "red" lw 2
set style line 9 lt 2 lc rgb "#a0a0f0" lw 0.5

plot \
     "< test/tools/gnuplotitems.pl ftri output/results/res-reach.txt" using 1:2 with lines ls 1 title "border", \
     "< test/tools/gnuplotitems.pl ftri output/results/res-reach.txt" using 1:2 with filledcurve ls 2 title "triangles", \
     "< test/tools/gnuplotitems.pl fcorner output/results/res-reach.txt" using 1:2 with points ls 4 title "corner"
pause -1

set zrange [1:]
set pm3d map
splot \
      "output/results/terrain.txt" using 1:2:4 with pm3d notitle
pause -1
