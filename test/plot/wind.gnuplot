# GNUplot script for wind.sh; read that shell script for details

set terminal png size 1024,768

set key outside right

set xlabel "time"
set xdata time
set format x "%H:%M:%S"
set timefmt "%H:%M:%S"

set ylabel "direction[deg]"
set yrange [ 0 : 360 ]

set output 'direction.png'
plot \
     'external.dat' using 1:2 with lines lc rgb "green" title "Butterfly Vario", \
     'circling.dat' using 1:2 with lines lw 2 lc rgb "red" title "XCSoar Circling", \
     'ekf.dat' using 1:2 with lines lw 2 lc rgb "blue" title "XCSoar EKF"


set ylabel "speed[m/s]"
 set yrange [ * : * ]

set output 'speed.png'
plot \
     'external.dat' using 1:3 with lines lc rgb "green" title "Butterfly Vario", \
     'circling.dat' using 1:3 with lines lw 2 lc rgb "red" title "XCSoar Circling", \
     'ekf.dat' using 1:3 with lines lw 2 lc rgb "blue" title "XCSoar EKF"

