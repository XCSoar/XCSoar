set size ratio -1
set key outside right

plot 'output/results/res-sample.txt' using 2:3 with lines title "path", \
	'./output/results/res-retro.txt' using 1:2 with lines title "retro"
pause -1
