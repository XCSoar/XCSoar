set terminal png size 1024,1024 crop
set output '| display png:-'

set size ratio -1
set key outside right

set style line 1 lt 2 lc rgb "black" lw 2
set style line 2 lt 4 lc rgb "purple" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1
set style line 4 lt 4 lc rgb "blue" lw 2
set style line 5 lt 4 lc rgb "grey" lw 2
set style line 6 lt 4 lc rgb "green" lw 1
set style line 7 lt 4 lc rgb "cyan" lw 1
set style line 8 lt 2 lc rgb "red" lw 2
set style line 9 lt 2 lc rgb "#a0a0f0" lw 0.5

plot \
     "< test/tools/gnuplotitems.pl polygon output/results/route.txt" using 1:2 with filledcurve ls 5 title "as", \
     'output/results/res-bb-in.txt' using 1:2 with lines ls 3 title "all airspace", \
     "< test/tools/gnuplotitems.pl spv output/results/route.txt" using 1:2 with lines ls 6 title "clearance", \
     "< test/tools/gnuplotitems.pl clear output/results/route.txt" using 1:2 with lines ls 8 title "clear", \
     "< test/tools/gnuplotitems.pl path output/results/route.txt" using 1:2 with lines ls 7 title "path", \
     "< test/tools/gnuplotitems.pl solution output/results/route.txt" using 1:2 with lines ls 1 title "solution", \
     "< test/tools/gnuplotitems.pl shortcut output/results/route.txt" using 1:2 with points ls 8 title "shortcut", \
     "< test/tools/gnuplotitems.pl terminal output/results/route.txt" using 1:2 with linespoints ls 2 title "terminal", \
     "< test/tools/gnuplotitems.pl check output/results/route.txt" using 1:2 with points ls 4 title "check"

set pm3d map
splot \
      "output/results/terrain.txt" using 1:2:3 with pm3d notitle, \
     'output/results/res-bb-in.txt' using 1:2:(0) with lines ls 3 title "all airspace", \
     "< test/tools/gnuplotitems.pl solution output/results/route.txt" using 1:2:3 with lines ls 1 title "solution"

#set pm3d explicit at s depthorder hidden3d 9
set terminal wxt
set output
unset border
unset key
unset xtics
unset ytics
unset ztics
unset xlabel
unset ylabel
unset colorbox
set parametric
#unset surface
set hidden3d
set view 28,306,1,0.2
set palette rgbformulae 8, 9, 7

splot \
      "output/results/terrain.txt" using 1:2:3 with lines, \
     'output/results/res-bb-in.txt' using 1:2:3 with lines ls 3 title "all airspace", \
     "< test/tools/gnuplotitems.pl solution output/results/route.txt" using 1:2:($3+300) with linespoints ls 1 title "solution"
pause -1
