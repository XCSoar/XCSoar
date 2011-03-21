set size ratio -1
set key left

set style line 1 lt 2 lc rgb "black" lw 2
set style line 2 lt 4 lc rgb "purple" lw 1
set style line 3 lt 4 lc rgb "orange" lw 1
set style line 4 lt 4 lc rgb "blue" lw 2
set style line 5 lt 4 lc rgb "grey" lw 2
set style line 6 lt 4 lc rgb "green" lw 1
set style line 7 lt 4 lc rgb "cyan" lw 1
set style line 8 lt 2 lc rgb "red" lw 2

set terminal png
set output 'results/plotres-1.png'
set xlabel "Longitude (deg)"
set ylabel "Latitude (deg)"
plot \
     'results/res-task.txt' using 1:2 with lines ls 6 title "OZ", \
     'results/res-ssample.txt' using 1:2 with filledcurve ls 8 title "sect samples", \
     'results/res-min.txt' using 1:2 with linespoints ls 3 title "min", \
     'results/res-max.txt' using 1:2 with linespoints ls 2 title "max", \
     'results/res-rem.txt' using 1:2 with linespoints ls 5 title "remaining", \
     'results/res-sample.txt' using 2:3 with lines ls 1 title "sample", \
     'results/res-isolines.txt' using 1:2 with lines ls 4 title "isolines"

set size noratio
set autoscale
unset xrange
unset yrange
set output 'results/plotres-2.png'
set multiplot layout 3, 1

set xlabel "t (s)"
set ylabel "Nav alt (m)"
plot \
     'results/res-sample.txt' using 1:4 with lines notitle

set ylabel "D remaining (m)"
plot \
     'results/res-stats.txt' using 1:4 with lines title "Effective", \
     'results/res-stats.txt' using 1:5 with lines title "Actual"

set ylabel "V (m/s)"
plot \
     'results/res-stats.txt' using 1:7 with lines title "Remaining", \
     'results/res-stats.txt' using 1:8 with lines title "Remaining instantaneous", \
     'results/res-stats.txt' using 1:9 with lines title "Effective", \
     'results/res-stats.txt' using 1:10 with lines title "Effective instantaneous", \
     'results/res-stats.txt' using 1:13 with lines title "Pirker"

unset multiplot

set output 'results/plotres-3.png'
set multiplot layout 2, 1

set autoscale
set ylabel " "
set title "Cruise efficiency"
plot \
     'results/res-stats.txt' using 1:2 with lines title "Active task point", \
     'results/res-stats.txt' using 1:6 with lines title "Cruise efficiency"

set title "Vario"
set yrange [-1:]
set ylabel "w,m (m/s)"
plot \
     'results/res-stats.txt' using 1:11 with lines title "Task vario", \
     'results/res-stats.txt' using 1:12 with lines title "Effective mc", \
     'results/res-stats.txt' using 1:3 with lines title "Auto mc"

unset multiplot

