set yrange [-1:14]
set size ratio -1

set style line 1 lt 2 lc rgb "black" lw 2
set style line 2 lt 4 lc rgb "green" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1

plot \
     'res-task.txt' using 1:2 with filledcurve title "OZ", \
     'res-min.txt' using 1:2 with linespoints ls 3 title "min", \
     'res-max.txt' using 1:2 with linespoints ls 2 title "max", \
     'res-sample.txt' using 1:2 with lines ls 1 title "sample"
pause -1


