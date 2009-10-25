set xrange [-0.1:1.6]
set size ratio -1

set style line 1 lt 2 lc rgb "black" lw 2
set style line 2 lt 4 lc rgb "purple" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1
set style line 4 lt 4 lc rgb "blue" lw 2
set style line 5 lt 4 lc rgb "grey" lw 2

plot 'res-bb-in.txt' using 1:2 with lines ls 3 title "in", \
     'res-bb-range.txt' using 1:2 with filledcurve ls 4 title "range", \
     'res-bb-range.txt' using 1:2 with points ls 4 notitle, \
     'res-bb-nearest.txt' using 1:2 with linespoints ls 2 title "nearest", \
     'res-bb-target.txt' using 1:2 with linespoints ls 5 title "search", \
     'res-sample.txt' using 1:2 with lines ls 1 title "sample"
pause -1
