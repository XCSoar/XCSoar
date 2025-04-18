
set logscale xy
set xlabel "n elements"
set ylabel "intersection queries"
plot 'output/results/res-tree-wp.txt' using 1:2 with lines title "waypoints", \
     'output/results/res-tree-as.txt' using 1:2 with lines title "airspace", \
        x title "1"
pause -1

set ylabel "speedup"
plot 'output/results/res-tree-wp.txt' using 1:($2/$1) with lines title "waypoints", \
     'output/results/res-tree-as.txt' using 1:($2/$1) with lines title "airspace", \
        1 title "1"
pause -1
