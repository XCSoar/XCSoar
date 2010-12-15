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

#set terminal png
#set output "route.png"
plot \
        "< test/tools/gnuplotitems.pl polygon results/route.txt" using 1:2 with filledcurve ls 5 title "as", \
     'results/res-bb-in.txt' using 1:2 with lines ls 3 title "all airspace", \
     "< test/tools/gnuplotitems.pl spv results/route.txt" using 1:2 with lines ls 6 title "clearance", \
     "< test/tools/gnuplotitems.pl clear results/route.txt" using 1:2 with lines ls 8 title "clear", \
     "< test/tools/gnuplotitems.pl path results/route.txt" using 1:2 with lines ls 7 title "path", \
     "< test/tools/gnuplotitems.pl solution results/route.txt" using 1:2 with lines ls 1 title "solution", \
     "< test/tools/gnuplotitems.pl terminal results/route.txt" using 1:2 with linespoints ls 2 title "terminal", \
     "< test/tools/gnuplotitems.pl check results/route.txt" using 1:2 with points ls 4 title "check"

pause -1
