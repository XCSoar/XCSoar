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
     'output/results/res-bb-in.txt' using 1:2 with lines ls 3 title "all airspace", \
     'output/results/res-bb-range.txt' using 1:2 with filledcurve ls 5 title "in range", \
     'output/results/res-bb-inside.txt' using 1:2 with filledcurve ls 8 title "inside airspace", \
     'output/results/res-sample.txt' using 2:3 with lines ls 1 title "sample"
pause -1

plot \
     'output/results/res-bb-range.txt' using 1:2 with filledcurve ls 5 title "in range", \
     'output/results/res-bb-sortednearest.txt' using 1:2 with filledcurve ls 2 title "sorted nearest", \
     'output/results/res-bb-range.txt' using 1:2 with lines ls 4 title "", \
     'output/results/res-bb-sortedsoonest.txt' using 1:2 with lines ls 8 title "sorted soonest", \
     'output/results/res-bb-closest.txt' using 1:2 with lines ls 3 title "closest"
pause -1

plot \
     'output/results/res-bb-in.txt' using 1:2 with lines ls 5 title "all airspace", \
     'output/results/res-bb-intersected.txt' using 1:2 with filledcurve ls 3 title "airspace", \
     'output/results/res-bb-intersected.txt' using 1:2 with lines ls 5 title "airspace", \
     'output/results/res-bb-intersects.txt' using 1:2 with linespoints ls 4 title "intersects", \
     'output/results/res-bb-intercepts.txt' using 1:2 with points ls 1 title "intercepts"

pause -1

set title "Airspace Warning Manager"
plot \
     'output/results/res-bb-in.txt' using 1:2 with lines ls 5 title "all airspace", \
     'output/results/res-as-warnings-inside.txt' using 1:2 with lines ls 8 title "inside", \
     'output/results/res-as-warnings-glide.txt' using 1:2 with lines ls 3 title "glide", \
     'output/results/res-as-warnings-filter.txt' using 1:2 with lines ls 2 title "filter", \
     'output/results/res-as-warnings-task.txt' using 1:2 with lines ls 4 title "Task", \
     'output/results/res-sample.txt' using 2:3 with lines ls 1 title "sample"
pause -1
