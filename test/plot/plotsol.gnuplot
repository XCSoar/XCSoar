set title "Remaining"
plot \
     'res-sol-remaining.txt' using 1:3 with filledcurve x1 title "min", \
     'res-sol-remaining.txt' using 1:4 with linespoints ls 1 title "elev", \
     'res-sol-remaining.txt' using 1:2 with lines ls 2 title "ac"
pause -1

set title "Planned"
plot \
     'res-sol-planned.txt' using 1:3 with filledcurve x1 title "min", \
     'res-sol-planned.txt' using 1:4 with linespoints ls 1 title "elev", \
     'res-sol-planned.txt' using 1:2 with lines ls 2 title "ac planned", \
     'res-sol-remaining.txt' using 1:2 with lines ls 3 title "ac act"
pause -1

