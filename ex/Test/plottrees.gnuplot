
set logscale xy
set xlabel "n elements"
set ylabel "intersection queries"
set nokey
plot 'results/res-trees.txt' using 1:2 with lines
pause -1

set ylabel "speedup"
set nokey
plot 'results/res-trees.txt' using 1:($2/$1) with lines
pause -1
