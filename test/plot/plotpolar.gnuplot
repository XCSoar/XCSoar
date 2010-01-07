
set nokey
set title "Glide polar at M=1.7"
set xlabel "V (m/s)"
set ylabel "w (m/s)"
plot 'results/res-polar.txt' using 2:($3) with lines, \
  'results/res-polar-17-best.txt' using 2:($3) with linespoints, \
  'results/res-polar-m.txt' using 4:(-$5) with points, \
  'results/res-polar-m.txt' using 6:(-$7) with points
pause -1

set ylabel "LD"
plot 'results/res-polar.txt' using 2:4 with lines, \
  'results/res-polar-17-best.txt' using 2:4 with linespoints
pause -1

set title "Optimal solution variation with mc"
set xlabel "M (m/s)"
set ylabel "VbestLD (m/s)"
plot 'results/res-polar-m.txt' using 1:2 with lines notitle
pause -1

set xlabel "M (m/s)"
set ylabel "LD"
plot 'results/res-polar-m.txt' using 1:3 with lines notitle
pause -1

set key
set title "Glide solution variation with aircraft alt"
set xlabel "h (m)"
set ylabel "alt diff (m)"
plot 'results/res-polar-h-00.txt' using 1:2 with lines title "W=0.0", \
     'results/res-polar-h-50.txt' using 1:2 with lines title "W=5.0"
pause -1

set ylabel "time elapsed (s)"
plot 'results/res-polar-h-00.txt' using 1:3 with lines title "W=0.0", \
     'results/res-polar-h-50.txt' using 1:3 with lines title "W=5.0"
pause -1

set ylabel "V opt (m/s)"
plot 'results/res-polar-h-00.txt' using 1:4 with lines title "W=0.0", \
     'results/res-polar-h-50.txt' using 1:4 with lines title "W=5.0"
pause -1


set nokey
set title "Glide solution variation with wind speed"

set xlabel "W (m/s)"
set ylabel "time elapsed (s)"
plot 'results/res-polar-w.txt' using 5:3 with lines
pause -1

set ylabel "V opt (m/s)"
plot 'results/res-polar-w.txt' using 5:4 with lines
pause -1

set nokey
set title "Glide solution variation with wind angle, W=10.0 m/s"

set xlabel "Wangle (deg)"
set ylabel "time elapsed (s)"
plot 'results/res-polar-a.txt' using 6:3 with lines
pause -1

set ylabel "V opt (m/s)"
plot 'results/res-polar-a.txt' using 6:4 with lines
pause -1

set key
set title "Speed to fly"
set xlabel "h (m)"
plot 'results/res-polar-s0.txt' using 1:4 with lines title "STF", \
     'results/res-polar-s0.txt' using 1:3 with lines title "VOpt"
pause -1

set xlabel "S gross (m/s)"
plot 'results/res-polar-s1.txt' using 7:4 with lines title "STF", \
     'results/res-polar-s1.txt' using 7:3 with lines title "VOpt"
pause -1

set xlabel "S gross (m/s)"
plot 'results/res-polar-s1.txt' using 7:4 with lines title "STF (below) no wind", \
     'results/res-polar-s2.txt' using 7:4 with lines title "STF (above) no wind", \
     'results/res-polar-s3.txt' using 7:4 with lines title "STF (below) wind 10 m/s", \
     'results/res-polar-s4.txt' using 7:4 with lines title "STF (above) wind 10 m/s"
pause -1

set xlabel "Wangle (deg)"
set ylabel "bearing (deg)"
plot 'results/res-polar-cb.txt' using 2:3 with lines title "target bearing", \
     'results/res-polar-cb.txt' using 2:4 with lines title "cruise track bearing"
pause -1


