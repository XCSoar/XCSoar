set xrange [-1:21]
set yrange [-1:11]
plot 'res-task.txt' using 1:2 with linespoints title "OZ", \
     'res-max.txt' using 1:2 with linespoints title "max", \
     'res-min.txt' using 1:2 with linespoints title "min"
pause -1


