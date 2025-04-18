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

set xlabel "Longitude (deg)"
set ylabel "Latitude (deg)"
plot \
     'output/results/res-sample.txt' using 2:3 with lines ls 1 title "sample", \
     'output/results/res-trace.txt' using 2:3 with lines ls 8 title "trace", \
     'output/results/res-trace-thin.txt' using 2:3 with linespoints ls 3 title "thin trace"
pause -1

set size noratio
set ylabel "vario (m/s)"
set xlabel "time (s)"
plot \
     'output/results/res-trace.txt' using 1:6 with lines ls 8 title "trace", \
     'output/results/res-trace-thin.txt' using 1:6 with lines ls 3 title "thin trace"
pause -1
