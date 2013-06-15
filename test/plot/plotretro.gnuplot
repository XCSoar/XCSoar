set size ratio -1
set key outside right

plot 'output/results/res-sample.txt' using 2:3 with lines title "path", \
	'./output/results/res-retro.txt' using 1:2 with linespoints title "retro", \
	'./output/results/res-retro.txt' using 1:2:3 with labels notitle
pause -1
