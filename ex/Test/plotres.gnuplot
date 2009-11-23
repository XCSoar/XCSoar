set size ratio -1

set style line 1 lt 2 lc rgb "black" lw 2
set style line 2 lt 4 lc rgb "purple" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1
set style line 4 lt 4 lc rgb "blue" lw 2
set style line 5 lt 4 lc rgb "grey" lw 2
set style line 6 lt 4 lc rgb "green" lw 1
set style line 7 lt 4 lc rgb "cyan" lw 1
set style line 8 lt 2 lc rgb "red" lw 2


plot \
     'results/res-task.txt' using 1:2 with lines ls 6 title "OZ", \
     'results/res-ssample.txt' using 1:2 with filledcurve ls 8 title "sect samples", \
     'results/res-bb-inside.txt' using 1:2 with lines ls 7 title "airspace", \
     'results/res-min.txt' using 1:2 with linespoints ls 3 title "min", \
     'results/res-max.txt' using 1:2 with linespoints ls 2 title "max", \
     'results/res-rem.txt' using 1:2 with linespoints ls 5 title "remaining", \
     'results/res-abort-task.txt' using 1:2 with linespoints ls 6 title "abort", \
     'results/res-sample.txt' using 2:3 with lines ls 1 title "sample", \
     'results/res-isolines.txt' using 1:2 with lines ls 4 title "isolines"
pause -1

set size noratio
set autoscale
unset xrange
unset yrange

set title "Stats - Altitude"
plot \
     'results/res-sample.txt' using 1:4 with lines title "ac alt"
pause -1


set title "Stats - Task Distance"
plot \
     'results/res-stats.txt' using 1:4 with lines title "dist eff rem", \
     'results/res-stats.txt' using 1:5 with lines title "dist eff act"
pause -1

set title "Stats - Misc"
plot \
     'results/res-stats.txt' using 1:2 with lines title "active task point", \
     'results/res-stats.txt' using 1:3 with lines title "mc_best", \
     'results/res-stats.txt' using 1:6 with lines title "cruise efficiency"
pause -1


set yrange [0:50]
set title "Stats - Speed"
plot \
     'results/res-stats.txt' using 1:7 with lines title "total rem sp", \
     'results/res-stats.txt' using 1:8 with lines title "total rem sp inst", \
     'results/res-stats.txt' using 1:9 with lines title "total eff sp", \
     'results/res-stats.txt' using 1:10 with lines title "total eff sp inst"
pause -1


