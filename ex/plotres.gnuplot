set yrange [-1:14]
set size ratio -1

set style line 1 lt 2 lc rgb "black" lw 2
set style line 2 lt 4 lc rgb "purple" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1

plot \
     'res-task.txt' using 1:2 with filledcurve title "OZ", \
     'res-ssample.txt' using 1:2 with filledcurve title "sect samples", \
     'res-min.txt' using 1:2 with linespoints ls 3 title "min", \
     'res-max.txt' using 1:2 with linespoints ls 2 title "max", \
     'res-sample.txt' using 1:2 with lines ls 1 title "sample"
pause -1

set size noratio
set autoscale
unset xrange
unset yrange
set title "Remaining"
plot \
     'res-sol-remaining.txt' using 1:3 with filledcurve x1 title "min", \
     'res-sol-remaining.txt' using 1:4 with linespoints ls 1 title "elev", \
     'res-sol-remaining.txt' using 1:2 with lines ls 2 title "ac"
pause -1

set title "Planned"
plot \
     'res-sol-planned.txt' using 1:3 with filledcurve x1 title "min", \
     'res-sol-planned.txt' using 1:4 with linespoints ls 1 title "elev", \
     'res-sol-planned.txt' using 1:2 with lines ls 2 title "ac planned", \
     'res-sol-remaining.txt' using 1:2 with lines ls 3 title "ac act"
pause -1

set title "Stats - Task Distance"
plot \
     'res-stats.txt' using 1:4 with lines title "dist eff rem", \
     'res-stats.txt' using 1:5 with lines title "dist eff act"
pause -1

set title "Stats - Misc"
plot \
     'res-stats.txt' using 1:2 with lines title "active task point", \
     'res-stats.txt' using 1:3 with lines title "mc_best", \
     'res-stats.txt' using 1:6 with lines title "cruise efficiency"
pause -1


set yrange [0:50]
set title "Stats - Speed"
plot \
     'res-stats.txt' using 1:7 with lines title "total rem sp", \
     'res-stats.txt' using 1:8 with lines title "total rem sp inst", \
     'res-stats.txt' using 1:9 with lines title "total eff sp", \
     'res-stats.txt' using 1:10 with lines title "total eff sp inst"
pause -1


