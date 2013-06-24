
set style data lines
set yrange [0:]
set xrange [0:]

set title "Range tolerance variation"

set xlabel "range tolerance (km)"
set ylabel "score dist (km)"
plot 'output/results/opt_retrospective-range.log' using ($1/1000):($3/1000) title "ach", \
	'' using ($1/1000):($4/1000) title "can"
pause -1

set xlabel "n"
plot 'output/results/opt_retrospective-range.log' using ($6):($3/1000) title "ach", \
	'' using ($6):($4/1000) title "can"
pause -1

set xlabel "range tolerance (km)"
set ylabel "n"
plot 'output/results/opt_retrospective-range.log' using ($1/1000):($6) notitle
pause -1

set title "Angle tolerance variation"

set xlabel "angle tolerance (deg)"
set ylabel "score dist (km)"
plot 'output/results/opt_retrospective-angle.log' using ($2):($3/1000) title "ach", \
	'' using ($2):($4/1000) title "can"
pause -1

set xlabel "n"
plot 'output/results/opt_retrospective-angle.log' using ($6):($3/1000) title "ach", \
	'' using ($6):($4/1000) title "can"
pause -1

set xlabel "angle tolerance (deg)"
set ylabel "n"
plot 'output/results/opt_retrospective-angle.log' using ($2):($6) notitle
pause -1
