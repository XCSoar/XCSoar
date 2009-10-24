set size ratio -1
set xrange [-250:500]
plot 'res-bb-in.txt' using 1:2 with lines title "in", \
     'res-bb-filtered.txt' using 1:2 with filledcurve title "filtered", \
     'res-bb-range.txt' using 1:2 with filledcurve title "range", \
     'res-bb-range.txt' using 1:2 with points notitle, \
     'res-bb-nearest.txt' using 1:2 with linespoints title "nearest", \
     'res-bb-target.txt' using 1:2 with linespoints title "search"
pause -1
