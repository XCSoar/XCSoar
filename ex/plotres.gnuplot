set yrange [-1:14]
set size ratio -1
plot 'res-task.txt' using 1:2 with linespoints title "OZ", \
     'res-max.txt' using 1:2 with linespoints title "max", \
     'res-min.txt' using 1:2 with linespoints title "min"
pause -1


