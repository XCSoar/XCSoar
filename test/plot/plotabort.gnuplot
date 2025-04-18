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


plot \
     'output/results/res-task.txt' using 1:2 with lines ls 6 title "OZ", \
     'output/results/res-rem.txt' using 1:2 with linespoints ls 5 title "remaining", \
     'output/results/res-abort-task.txt' using 1:2 with linespoints ls 6 title "abort", \
     'output/results/res-sample.txt' using 2:3 with lines ls 1 title "sample"
pause -1

